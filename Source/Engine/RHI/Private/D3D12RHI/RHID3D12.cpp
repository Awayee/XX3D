#include "RHID3D12.h"
#include "Resource/Public/Config.h"
#include "D3D12Util.h"

namespace Engine {
#ifdef _DEBUG
	void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
	{
		UINT count = 0;
		UINT flags = 0;

		// Call with nullptr to get list count.
		output->GetDisplayModeList(format, flags, &count, nullptr);

		std::vector<DXGI_MODE_DESC> modeList(count);
		output->GetDisplayModeList(format, flags, &count, &modeList[0]);

		for (auto& x : modeList)
		{
			UINT n = x.RefreshRate.Numerator;
			UINT d = x.RefreshRate.Denominator;
			std::wstring text =
				L"Width = " + std::to_wstring(x.Width) + L" " +
				L"Height = " + std::to_wstring(x.Height) + L" " +
				L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
				L"\n";

			wprintf(text.c_str());
		}
	}
	void LogAdapterOutputs(IDXGIAdapter* adapter, DXGI_FORMAT format)
	{
		UINT i = 0;
		IDXGIOutput* output = nullptr;
		while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
		{
			DXGI_OUTPUT_DESC desc;
			output->GetDesc(&desc);

			std::wstring text = L"***Output: ";
			text += desc.DeviceName;
			text += L"\n";
			wprintf(text.c_str());

			LogOutputDisplayModes(output, format);

			DX_RELEASE(output);

			++i;
		}
	}

	void LogAdapters(IDXGIFactory4* factory, DXGI_FORMAT format) {
		UINT i = 0;
		IDXGIAdapter* adapter = nullptr;
		TVector<IDXGIAdapter*> adapterList;
		while (factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
		{
			DXGI_ADAPTER_DESC desc;
			adapter->GetDesc(&desc);

			std::wstring text = L"***Adapter: ";
			text += desc.Description;
			text += L"\n";
			wprintf(text.c_str());
			adapterList.PushBack(adapter);
			++i;
		}

		//for (size_t i = 0; i < adapterList.Size(); ++i)
		//{
		//	LogAdapterOutputs(adapterList[i], format);
		//	DX_RELEASE(adapterList[i]);
		//}
	}
#endif

	void RHID3D12::CreateDeviceContext() {
#ifdef _DEBUG
		// Enable the D3D12 debug layer.
		{
			ID3D12Debug* debugController;
			DX_CHECK(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
			debugController->EnableDebugLayer();
		}
#endif
		DX_CHECK(CreateDXGIFactory1(IID_PPV_ARGS(&m_DXGIFactory)));

		// Try to create hardware device.
		HRESULT hardwareResult = D3D12CreateDevice(
			nullptr,             // default adapter
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_Device));

		// Fallback to WARP device.
		if (FAILED(hardwareResult))
		{
			IDXGIAdapter* pWarpAdapter;
			DX_CHECK(m_DXGIFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

			DX_CHECK(D3D12CreateDevice(
				pWarpAdapter,
				D3D_FEATURE_LEVEL_11_0,
				IID_PPV_ARGS(&m_Device)));
		}

		DX_CHECK(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
			IID_PPV_ARGS(&m_Fence)));

		m_RtvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// Check 4X MSAA quality support for our back buffer format.
		// All Direct3D 11 capable devices support 4X MSAA for all render 
		// target formats, so we only need to check quality support.
		DXGI_FORMAT swapchainFormat = ConvertD3D12Format(m_SwapchainFormat);
		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
		msQualityLevels.Format = swapchainFormat;
		msQualityLevels.SampleCount = 4;
		msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
		msQualityLevels.NumQualityLevels = 0;
		DX_CHECK(m_Device->CheckFeatureSupport(
			D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
			&msQualityLevels,
			sizeof(msQualityLevels)));

		m_MsaaQuality = msQualityLevels.NumQualityLevels;
		ASSERT(m_MsaaQuality > 0,  "Unexpected MSAA quality level.");

#ifdef _DEBUG
		LogAdapters(m_DXGIFactory, swapchainFormat);
#endif
	}

	void RHID3D12::CreateCommandObjects() {
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		DX_CHECK(m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue)));

		DX_CHECK(m_Device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&m_CommandAllocator)));

		DX_CHECK(m_Device->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			m_CommandAllocator, // Associated command allocator
			nullptr,                   // Initial PipelineStateObject
			IID_PPV_ARGS(&m_GraphicsCommandList)));

		// Start off in a closed state.  This is because the first time we refer 
		// to the command list we will Reset it, and it needs to be closed before
		// calling Reset.
		m_GraphicsCommandList->Close();
	}

	void RHID3D12::CreateSwapchain(HWND wnd) {
		// Release the previous swapchain we will be recreating.
		DX_RELEASE(m_Swapchain);

		DXGI_SWAP_CHAIN_DESC sd;
		sd.BufferDesc.Width = m_SwapchainExtent.w;
		sd.BufferDesc.Height = m_SwapchainExtent.h;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferDesc.Format = ConvertD3D12Format(m_SwapchainFormat);
		sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		sd.SampleDesc.Count = m_EnableMsaa ? m_MsaaSampleCount : 1;
		sd.SampleDesc.Quality = m_EnableMsaa ? (m_MsaaQuality - 1) : 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = BACK_BUFFER_COUNT;
		sd.OutputWindow = wnd;
		sd.Windowed = true;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		// Note: Swap chain uses queue to perform flush.
		DX_CHECK(m_DXGIFactory->CreateSwapChain(
			m_CommandQueue,
			&sd,
			&m_Swapchain));
	}

	void RHID3D12::CreateInternalDescriptorHeaps() {
		// rtv
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
		rtvHeapDesc.NumDescriptors = BACK_BUFFER_COUNT;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		rtvHeapDesc.NodeMask = 0;
		DX_CHECK(m_Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RTVDescriptorHeap)));

		// dsv
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		dsvHeapDesc.NodeMask = 0;
		DX_CHECK(m_Device->CreateDescriptorHeap( &dsvHeapDesc, IID_PPV_ARGS(&m_DSVDescriptorHeap)));
	}

	void RHID3D12::FlushCommandQueue() {
		// Advance the fence value to mark commands up to this fence point.
		m_CurrentFence++;

		// Add an instruction to the command queue to set a new fence point.  Because we 
		// are on the GPU timeline, the new fence point won't be set until the GPU finishes
		// processing all the commands prior to this Signal().
		DX_CHECK(m_CommandQueue->Signal(m_Fence, m_CurrentFence));

		// Wait until the GPU has completed commands up to this fence point.
		if (m_Fence->GetCompletedValue() < m_CurrentFence)
		{
			HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

			// Fire event when GPU hits current fence.  
			DX_CHECK(m_Fence->SetEventOnCompletion(m_CurrentFence, eventHandle));

			// Wait until the GPU hits current fence event is fired.
			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}
	}

	void RHID3D12::OnResized() {
		ASSERT(m_Device && m_Swapchain && m_CommandAllocator, "");

		// Release the previous resources we will be recreating.
		for (int i = 0; i < BACK_BUFFER_COUNT; ++i) {
			DX_RELEASE(m_SwapchainBuffer[i]);
		}

		// Resize the swap chain.
		DX_CHECK(m_Swapchain->ResizeBuffers(
			BACK_BUFFER_COUNT,
			m_SwapchainExtent.w, m_SwapchainExtent.h,
			ConvertD3D12Format(m_SwapchainFormat),
			DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

		m_CurrentBackBuffer = 0;

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		for (UINT i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			DX_CHECK(m_Swapchain->GetBuffer(i, IID_PPV_ARGS(&m_SwapchainBuffer[i])));
			m_Device->CreateRenderTargetView(m_SwapchainBuffer[i], nullptr, rtvHeapHandle);
			rtvHeapHandle.Offset(1, m_RtvDescriptorSize);
		}

		// Update the viewport transform to cover the client area.
		m_Viewport.TopLeftX = 0;
		m_Viewport.TopLeftY = 0;
		m_Viewport.Width = static_cast<float>(m_SwapchainExtent.w);
		m_Viewport.Height = static_cast<float>(m_SwapchainExtent.h);
		m_Viewport.MinDepth = 0.0f;
		m_Viewport.MaxDepth = 1.0f;

		m_ScissorRect = { 0, 0, static_cast<LONG>(m_SwapchainExtent.w), static_cast<LONG>(m_SwapchainExtent.h) };
	}

	void RHID3D12::InternalImmediateCommit(CmdFunc&& func) {
		DX_CHECK(m_CommandAllocator->Reset());
		DX_CHECK(m_GraphicsCommandList->Reset(m_CommandAllocator, nullptr));
		ID3D12GraphicsCommandList* cmdList = m_GraphicsCommandList;
		func(cmdList);
		DX_CHECK(m_GraphicsCommandList->Close());
		ID3D12CommandList* cmdListCommon = static_cast<ID3D12CommandList*>(cmdList);
		m_CommandQueue->ExecuteCommandLists(1, &cmdListCommon);
		FlushCommandQueue();
	}

	RHID3D12::RHID3D12(const RHIInitDesc& desc) {
		CreateDeviceContext();
		CreateCommandObjects();

		m_SwapchainExtent = desc.WindowSize;
		HWND wnd = reinterpret_cast<HWND>(desc.WindowHandle);
		CreateSwapchain(wnd);
		CreateInternalDescriptorHeaps();
		OnResized();
	}

	RHID3D12::~RHID3D12() {
		for (uint32 i = 0; i < BACK_BUFFER_COUNT; ++i) {
			DX_RELEASE(m_SwapchainBuffer[i]);
		}
		DX_RELEASE(m_DepthStencilBuffer);
		DX_RELEASE(m_CommandAllocator);
		DX_RELEASE(m_GraphicsCommandList);
		DX_RELEASE(m_RTVDescriptorHeap);
		DX_RELEASE(m_DSVDescriptorHeap);
		DX_RELEASE(m_CommandQueue);
		DX_RELEASE(m_Fence);
		DX_RELEASE(m_Swapchain);
		DX_RELEASE(m_Device);
	}
}
