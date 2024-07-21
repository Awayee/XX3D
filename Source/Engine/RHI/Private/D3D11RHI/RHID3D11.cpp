#include "RHID3D11.h"
#include "Resource/Public/Config.h"
#include "D3D11Util.h"

namespace Engine {

	void RHID3D11::CreateDeviceContext() {
		UINT createDeviceFlags = 0;
#ifdef _DEBUG
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
		TVector<D3D_DRIVER_TYPE> driverTypes{
			D3D_DRIVER_TYPE_HARDWARE,
			D3D_DRIVER_TYPE_WARP,
			D3D_DRIVER_TYPE_REFERENCE,
		};
		TVector<D3D_FEATURE_LEVEL> featureLevels = {
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
		};
		for(D3D_DRIVER_TYPE d3dDriverType: driverTypes) {
			D3D_FEATURE_LEVEL featureLevel;
			HRESULT hr = D3D11CreateDevice(nullptr, d3dDriverType, nullptr, createDeviceFlags, featureLevels.Data(), featureLevels.Size(), D3D11_SDK_VERSION, &m_Device, &featureLevel, &m_Context);
			if (hr == E_INVALIDARG){
				// Direct3D 11.0 的API不承认D3D_FEATURE_LEVEL_11_1，所以我们需要尝试特性等级11.0以及以下的版本
				hr = D3D11CreateDevice(nullptr, d3dDriverType, nullptr, createDeviceFlags, &featureLevels[1], featureLevels.Size() - 1,
					D3D11_SDK_VERSION, &m_Device, &featureLevel, &m_Context);
			}
			if (SUCCEEDED(hr))
				break;
		}
	}

	void RHID3D11::CreateSwapchain(HWND wnd) {
		//check msaa
		DXGI_FORMAT swapchainFormat = ConvertD3D11Format(m_SwapchainFormat);
		uint32 msaaSampleCount = ConfigManager::GetData().MSAASampleCount;
		uint32 msaaQuality;
		if (msaaSampleCount > 1) {
			m_Device->CheckMultisampleQualityLevels(swapchainFormat, msaaSampleCount, &msaaQuality);
		}
		else {
			msaaSampleCount = 1;
			msaaQuality = 1;
		}
		//create swapchain
		DXGI_SWAP_CHAIN_DESC swapchainDesc;
		swapchainDesc.BufferDesc.Width = m_SwapchainExtent.w; // use window's client area dims
		swapchainDesc.BufferDesc.Height = m_SwapchainExtent.h;
		swapchainDesc.BufferDesc.RefreshRate.Numerator = 60;
		swapchainDesc.BufferDesc.RefreshRate.Denominator = 1;
		swapchainDesc.BufferDesc.Format = swapchainFormat;
		swapchainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapchainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapchainDesc.SampleDesc.Count = msaaSampleCount;
		// m4xMsaaQuality is returned via CheckMultisampleQualityLevels().
		swapchainDesc.SampleDesc.Quality = msaaQuality - 1;
		swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapchainDesc.BufferCount = 1;//num of background buffer, just 1
		swapchainDesc.OutputWindow = wnd;
		swapchainDesc.Windowed = true;
		swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapchainDesc.Flags = 0;
		IDXGIDevice* dxgiDevice = 0;
		DX_CHECK(m_Device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice));
		IDXGIAdapter* dxgiAdapter = 0;
		DX_CHECK(dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter));
		// Finally got the IDXGIFactory interface.
		IDXGIFactory* dxgiFactory = 0;
		DX_CHECK(dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory));
		// Now, create the swap chain.
		DX_CHECK(dxgiFactory->CreateSwapChain(m_Device, &swapchainDesc, &m_Swapchain));
		// Release our acquired COM interfaces (because we are done with them).
		DX_RELEASE(dxgiDevice);
		DX_RELEASE(dxgiAdapter);
		DX_RELEASE(dxgiFactory);
	}

	void RHID3D11::OnResized() {
		if(!m_Swapchain) {
			return;
		}
		DX_RELEASE(m_RenderTargetView);

		// Resize the swap chain and recreate the render target view.sdf
		DX_CHECK(m_Swapchain->ResizeBuffers(1, m_SwapchainExtent.w, m_SwapchainExtent.h, ConvertD3D11Format(m_SwapchainFormat), 0));
		ID3D11Texture2D* backBuffer;
		DX_CHECK(m_Swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer)));
		DX_CHECK(m_Device->CreateRenderTargetView(backBuffer, 0, &m_RenderTargetView));
		DX_RELEASE(backBuffer);

		// Bind the render target view and depth/stencil view to the pipeline.
		//m_Context->OMSetRenderTargets(1, &m_RenderTargetView, m_DepthStencilView);

		// Set the viewport transform.
		m_Viewport.TopLeftX = 0;
		m_Viewport.TopLeftY = 0;
		m_Viewport.Width = static_cast<float>(m_SwapchainExtent.w);
		m_Viewport.Height = static_cast<float>(m_SwapchainExtent.h);
		m_Viewport.MinDepth = 0.0f;
		m_Viewport.MaxDepth = 1.0f;
		m_Context->RSSetViewports(1, &m_Viewport);
	}

	RHID3D11::RHID3D11(const RHIInitDesc* initDesc) {
		CreateDeviceContext();
		m_SwapchainExtent = initDesc->WindowSize;
		HWND wnd = reinterpret_cast<HWND>(initDesc->WindowHandle);
		CreateSwapchain(wnd);
		OnResized();
	}

	RHID3D11::~RHID3D11() {
		DX_RELEASE(m_RenderTargetView);
		DX_RELEASE(m_Swapchain);
		m_Context->ClearState();
		DX_RELEASE(m_Context);
		DX_RELEASE(m_Device);
	}
}
