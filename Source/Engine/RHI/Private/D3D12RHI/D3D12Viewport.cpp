#include "D3D12Viewport.h"
#include "D3D12command.h"
#include "D3d12Device.h"

D3D12BackBuffer::D3D12BackBuffer(const RHITextureDesc& desc) : D3D12Texture(desc), m_Resource(nullptr), m_Descriptor(InvalidCPUDescriptor()) {
	m_ResState.SetState(D3D12_RESOURCE_STATE_PRESENT);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12BackBuffer::GetDescriptor(ETexDescriptorType type, RHITextureSubRes subRes) {
	ASSERT(ETexDescriptorType::RTV == type, "Back buffer supports only RTV descriptor!");
	return m_Descriptor;
}

void D3D12BackBuffer::ResetResource(ID3D12Resource* resource, D3D12_CPU_DESCRIPTOR_HANDLE descriptor) {
	m_Resource = resource;
	m_Descriptor = descriptor;
}

D3D12Viewport::D3D12Viewport(IDXGIFactory4* factory, D3D12Device* device, WindowHandle window, USize2D windowSize, const RHIInitConfig& cfg) :
	m_DXGIFactory(factory), m_Device(device), m_Window(window), m_SwapchainExtent(windowSize), m_EnableVSync(cfg.EnableVSync), m_EnableMsaa(cfg.EnableMSAA) {
	// create fence for present complete
	m_PresentFence.Reset(new D3D12Fence(m_Device->GetDevice()));
	m_PresentFence->SetName("PresentComplete");
	// get graphics queue to present
	m_Queue = m_Device->GetCommandMgr()->GetQueue(EQueueType::Graphics);
	CreateSwapchain();
	OnResize();
}

D3D12Viewport::~D3D12Viewport() {
	StaticDescriptorAllocator* allocator = m_Device->GetDescriptorMgr()->GetStaticAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	for(auto& descriptor: m_BackBufferDescriptors) {
		allocator->FreeDescriptorSlot(descriptor);
	}
}

void D3D12Viewport::SetSize(USize2D size) {
	if(m_SwapchainExtent != size) {
		m_SwapchainExtent = size;
		OnResize();
	}
}

USize2D D3D12Viewport::GetSize() {
	return m_SwapchainExtent;
}

bool D3D12Viewport::PrepareBackBuffer() {
	m_PresentFence->Wait();
	m_PresentFence->Reset();
	UpdateBackBuffer();
	return true;
}

RHITexture* D3D12Viewport::GetBackBuffer() {
	return m_BackBuffer.Get();
}

ERHIFormat D3D12Viewport::GetBackBufferFormat() {
	return m_SwapchainFormat;
}

void D3D12Viewport::Present() {
	const LONG presentFlags = m_EnableVSync ? 0 : DXGI_PRESENT_ALLOW_TEARING;
	m_Swapchain->Present(!!m_EnableVSync, presentFlags);
	m_Queue->SignalFence(m_PresentFence);
	m_CurrentBackBuffer = (m_CurrentBackBuffer + 1) % BACK_BUFFER_COUNT;
}

void D3D12Viewport::CreateSwapchain() {
	// Release the previous swapchain we will be recreating.
	m_Swapchain.Reset();
	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = m_SwapchainExtent.w;
	sd.BufferDesc.Height = m_SwapchainExtent.h;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = ToD3D12Format(m_SwapchainFormat);
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = m_EnableMsaa ? m_MsaaSampleCount : 1;
	sd.SampleDesc.Quality = m_EnableMsaa ? (m_MsaaQuality - 1) : 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = BACK_BUFFER_COUNT;
	sd.OutputWindow = (HWND)m_Window;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = GetSwapchainFlags();
	// Note: Swap chain uses queue to perform flush.
	DX_CHECK(m_DXGIFactory->CreateSwapChain(m_Queue->GetCommandQueue(), &sd, m_Swapchain.Address()));

	// allocate back buffer descriptor heap
	StaticDescriptorAllocator* allocator = m_Device->GetDescriptorMgr()->GetStaticAllocator(	D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	for(uint32 i=0; i<BACK_BUFFER_COUNT; ++i) {
		m_BackBufferDescriptors[i] = allocator->AllocateDescriptorSlot();
	}

	// create back buffer
	RHITextureDesc backBufferDesc = RHITextureDesc::Texture2D();
	backBufferDesc.Format = m_SwapchainFormat;
	backBufferDesc.Width = m_SwapchainExtent.w;
	backBufferDesc.Height = m_SwapchainExtent.h;
	backBufferDesc.Flags = ETextureFlags::ColorTarget | ETextureFlags::Present;
	m_BackBuffer.Reset(new D3D12BackBuffer(backBufferDesc));
}

void D3D12Viewport::OnResize() {
	CHECK(m_Swapchain);
	// wait fence
	m_PresentFence->Wait();

	// Release the previous resources we will be recreating.
	for (auto& bf : m_BackBufferResources) {
		bf.Reset();
	}

	// check if the window is closed
	if(!::IsWindow((HWND)m_Window)) {
		return;
	}

	// Resize the swap chain.
	m_Swapchain->ResizeBuffers(
		BACK_BUFFER_COUNT,
		m_SwapchainExtent.w, m_SwapchainExtent.h,
		ToD3D12Format(m_SwapchainFormat),
		GetSwapchainFlags());
	m_CurrentBackBuffer = 0;
	StaticDescriptorAllocator* allocator = m_Device->GetDescriptorMgr()->GetStaticAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	for (UINT i = 0; i < BACK_BUFFER_COUNT; i++){
		DX_CHECK(m_Swapchain->GetBuffer(i, IID_PPV_ARGS(m_BackBufferResources[i].Address())));
		XWString resName = WStringFormat(L"BackBuffer%u", i);
		m_BackBufferResources[i]->SetName(resName.c_str());
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = allocator->GetCPUHandle(m_BackBufferDescriptors[i]);
		m_Device->GetDevice()->CreateRenderTargetView(m_BackBufferResources[i], nullptr, cpuHandle);
	}
	UpdateBackBuffer();
}

void D3D12Viewport::UpdateBackBuffer() {
	ID3D12Resource* resource = m_BackBufferResources[m_CurrentBackBuffer].Get();
	StaticDescriptorAllocator* allocator = m_Device->GetDescriptorMgr()->GetStaticAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = allocator->GetCPUHandle(m_BackBufferDescriptors[m_CurrentBackBuffer]);
	m_BackBuffer->ResetResource(resource, cpuHandle);
}

uint32 D3D12Viewport::GetSwapchainFlags() {
	uint32 flags = 0;
	if (!m_EnableVSync) {
		flags |= (DXGI_SWAP_CHAIN_FLAG)(DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
	}
	return flags;
}
