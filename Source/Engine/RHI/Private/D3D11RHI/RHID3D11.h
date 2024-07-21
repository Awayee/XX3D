#pragma once
#include "RHI\Public\RHI.h"
#include <d3d11.h>
namespace Engine {
	class RHID3D11 final: RHI {
	private:
		ID3D11Device*           m_Device{ nullptr };
		ID3D11DeviceContext*    m_Context{ nullptr };
		IDXGISwapChain*         m_Swapchain{ nullptr };
		ID3D11RenderTargetView* m_RenderTargetView{ nullptr };
		D3D11_VIEWPORT          m_Viewport{};
		USize2D                 m_SwapchainExtent{};
		ERHIFormat                 m_SwapchainFormat{ERHIFormat::B8G8R8A8_UNORM };
		ERHIFormat                 m_DepthFormat{ERHIFormat::D24_UNORM_S8_UINT };
		void CreateDeviceContext();
		void CreateSwapchain(HWND wnd);
		void OnResized();
	public:
		explicit RHID3D11(const RHIInitDesc* initInfo);
		~RHID3D11() override;
		ID3D11Device* Device() { return m_Device; }
		ID3D11DeviceContext* Context() { return m_Context; }

		static RHID3D11* InstanceD3D11() { return dynamic_cast<RHID3D11*>(RHI::Instance()); }
	};

#define DX_DEVICE  RHID3D11::InstanceD3D11()->Device()
#define DX_CONTEXT RHID3D11::InstanceD3D11()->Context()
}