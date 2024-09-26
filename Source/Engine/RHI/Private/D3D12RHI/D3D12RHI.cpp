#include "D3D12RHI.h"
#include "D3D12Command.h"
#include "D3D12Viewport.h"
#include "D3D12Descriptor.h"
#include "D3D12Device.h"
#include "D3D12Pipeline.h"
#include "D3D12Resources.h"
#include "System/Public/EngineConfig.h"

inline IDXGIAdapter* ChooseAdapter(IDXGIFactory4* factory) {
	uint32 i = 0;
	IDXGIAdapter* adapter = nullptr;
	const bool useIntegratedGPU = Engine::ConfigManager::GetData().UseIntegratedGPU;
	while(factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND) {
		DXGI_ADAPTER_DESC desc;
		adapter->GetDesc(&desc);
		// Assume that discrete GPU has 1GB video memory at least.
		constexpr size_t DISCRETE_GPU_VIDEO_MEMORY = 1 << 30;
		const bool isIntegratedGPU = desc.DedicatedVideoMemory < DISCRETE_GPU_VIDEO_MEMORY;
		if(isIntegratedGPU == useIntegratedGPU) {
			const XString adapterName = WString2String(desc.Description);
			LOG_DEBUG("Adapter found! %s", adapterName.c_str());
			return adapter;
		}
		adapter->Release();
		++i;
	}
	return nullptr;
}

#ifdef _DEBUG
void CALLBACK DebugOutputCallback(
	D3D12_MESSAGE_CATEGORY Category,
	D3D12_MESSAGE_SEVERITY Severity,
	D3D12_MESSAGE_ID ID,
	LPCSTR pDescription,
	void* pContext)
{
	if (D3D12_MESSAGE_SEVERITY_WARNING == Severity) {
		LOG_WARNING("[D3D12 Warning] %s", pDescription);
	}
	else if (D3D12_MESSAGE_SEVERITY_ERROR == Severity) {
		LOG_ERROR("[D3D12 Error] %s", pDescription);
	}
	else if (D3D12_MESSAGE_SEVERITY_CORRUPTION == Severity) {
		LOG_ERROR("[D3D12 Corruption] %s", pDescription);
	}
}
#endif

D3D12RHI::D3D12RHI(void* wnd, USize2D extent) {
	// Enable the D3D12 debug layer.
	if(RHIConfig::GetEnableDebug()){
		TDXPtr<ID3D12Debug> debug;
		DX_CHECK(D3D12GetDebugInterface(IID_PPV_ARGS(debug.Address())));
		debug->EnableDebugLayer();
	}
	DX_CHECK(CreateDXGIFactory1(IID_PPV_ARGS(m_DXGIFactory.Address())));

	// Choose adapter
	IDXGIAdapter* adapter = ChooseAdapter(m_DXGIFactory);
	if (!adapter) {
		LOG_ERROR("Could not find an adapter!");
	}
	m_Device.Reset(new D3D12Device(adapter));
	// Initialize debug callback
	if(RHIConfig::GetEnableDebug()){
		TDXPtr<ID3D12InfoQueue1> infoQueue;
		DX_CHECK(m_Device->GetDevice()->QueryInterface(IID_PPV_ARGS(infoQueue.Address())));
		DWORD callbackCookie;
		infoQueue->RegisterMessageCallback(DebugOutputCallback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &callbackCookie);

	}

	m_Viewport.Reset(new D3D12Viewport(m_DXGIFactory, m_Device, wnd, extent));
}

D3D12RHI::~D3D12RHI() {
	m_Viewport.Reset();
	m_Device.Reset();
	m_DXGIFactory.Reset();
}

void D3D12RHI::BeginFrame() {
	m_Device->GetCommandMgr()->BeginFrame();
	m_Device->GetUploader()->BeginFrame();
	m_Device->GetDescriptorMgr()->BeginFrame();
}

ERHIFormat D3D12RHI::GetDepthFormat() {
	return m_DepthFormat;
}

RHIViewport* D3D12RHI::GetViewport() {
	return m_Viewport.Get();
}

RHIBufferPtr D3D12RHI::CreateBuffer(const RHIBufferDesc& desc) {
	return RHIBufferPtr(new D3D12BufferImpl(desc, m_Device));
}

RHITexturePtr D3D12RHI::CreateTexture(const RHITextureDesc& desc) {
	return RHITexturePtr(new D3D12TextureImpl(desc, m_Device));
}

RHISamplerPtr D3D12RHI::CreateSampler(const RHISamplerDesc& desc) {
	return RHISamplerPtr(new D3D12Sampler(desc, m_Device));
}

RHIFencePtr D3D12RHI::CreateFence(bool isSignaled) {
	return RHIFencePtr(new D3D12Fence(m_Device->GetDevice()));
}

RHIShaderPtr D3D12RHI::CreateShader(EShaderStageFlags type, const char* codeData, uint32 codeSize, const XString& entryFunc) {
	return RHIShaderPtr(new D3D12Shader(type, codeData, codeSize));
}

RHIGraphicsPipelineStatePtr D3D12RHI::CreateGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc) {
	return RHIGraphicsPipelineStatePtr(new D3D12GraphicsPipelineState(desc, m_Device));
}

RHIComputePipelineStatePtr D3D12RHI::CreateComputePipelineState(const RHIComputePipelineStateDesc& desc) {
	return RHIComputePipelineStatePtr(new D3D12ComputePipelineState(desc, m_Device));
}

RHICommandBufferPtr D3D12RHI::CreateCommandBuffer(EQueueType queue) {
	return m_Device->GetCommandMgr()->GetQueue(queue)->CreateCommandList();
}

void D3D12RHI::SubmitCommandBuffers(TArrayView<RHICommandBuffer*> cmds, EQueueType queue, RHIFence* fence, bool bPresent) {
	TArrayView<D3D12CommandList*> d3d12Cmds{ (D3D12CommandList**)cmds.Data(), cmds.Size() };
	D3D12Queue* queuePtr = m_Device->GetCommandMgr()->GetQueue(queue);
	queuePtr->ExecuteCommandLists(d3d12Cmds);
	if(fence) {
		queuePtr->SignalFence((D3D12Fence*)fence);
	}
}

D3D12Device* D3D12RHI::GetDevice() {
	return m_Device;
}
