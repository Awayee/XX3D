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
		RFormat                 m_SwapchainFormat{ FORMAT_B8G8R8A8_UNORM };
		RFormat                 m_DepthFormat{ FORMAT_D24_UNORM_S8_UINT };
		void CreateDeviceContext();
		void CreateSwapchain(HWND wnd);
		void OnResized();
	public:
		RHID3D11() = default;
		~RHID3D11() = default;
		void Initialize(const RSInitInfo* initInfo) override;
		void Release() override;
		ID3D11Device* Device() { return m_Device; }
		ID3D11DeviceContext* Context() { return m_Context; }
		const USize2D& GetSwapchainExtent() override { return m_SwapchainExtent; }
		RFormat GetSwapchainImageFormat() override { return m_SwapchainFormat; }
		RFormat GetDepthFormat() override { return m_DepthFormat; }
		RImageView* GetSwapchainImageView(uint8 i) override;
		uint32 GetSwapchainMaxImageCount() override { return 1; }
		void ResizeSwapchain(uint32 width, uint32 height) override;

		static RHID3D11* InstanceD3D11() { return dynamic_cast<RHID3D11*>(RHI::Instance()); }
	};

#define DX_DEVICE  RHID3D11::InstanceD3D11()->Device()
#define DX_CONTEXT RHID3D11::InstanceD3D11()->Context()
}