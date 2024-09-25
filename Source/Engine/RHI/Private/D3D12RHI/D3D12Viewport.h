#pragma once
#include "D3D12Util.h"
#include "D3D12Descriptor.h"
#include "RHI/Public/RHI.h"
#include "D3D12Resources.h"

class D3D12Device;
class D3D12Queue;

class D3D12BackBuffer: public D3D12Texture {
public:
	D3D12BackBuffer(const RHITextureDesc& desc);
	void UpdateData(const void* data, uint32 byteSize, RHITextureSubRes subRes, IOffset3D offset) override { CHECK(0); }
	ID3D12Resource* GetResource() override { return m_Resource; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptor(ETexDescriptorType type, RHITextureSubRes subRes) override;
	void ResetResource(ID3D12Resource* resource, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);
private:
	ID3D12Resource* m_Resource;
	D3D12_CPU_DESCRIPTOR_HANDLE m_Descriptor;
};

class D3D12Viewport : public RHIViewport{
public:
	D3D12Viewport(IDXGIFactory4* factory, D3D12Device* device, WindowHandle window, USize2D windowSize);
	~D3D12Viewport() override;
	void SetSize(USize2D size) override;
	USize2D GetSize() override;
	bool PrepareBackBuffer() override;
	RHITexture* GetBackBuffer() override;
	ERHIFormat GetBackBufferFormat() override;
	void Present() override;
private:
	static const uint8 BACK_BUFFER_COUNT = 2;
	IDXGIFactory4* m_DXGIFactory;
	D3D12Device* m_Device;
	D3D12Queue* m_Queue;
	WindowHandle m_Window;
	USize2D m_SwapchainExtent;

	bool      m_EnableMsaa{ false };
	uint32    m_MsaaSampleCount{ 4 };
	uint32    m_MsaaQuality{ 0 };

	TDXPtr<IDXGISwapChain> m_Swapchain{ nullptr };
	TStaticArray<StaticDescriptorHandle, BACK_BUFFER_COUNT> m_BackBufferDescriptors;
	TStaticArray<TDXPtr<ID3D12Resource>, BACK_BUFFER_COUNT> m_BackBufferResources;
	uint32 m_CurrentBackBuffer{ 0 };
	TUniquePtr<D3D12BackBuffer> m_BackBuffer;
	ERHIFormat m_SwapchainFormat{ ERHIFormat::R8G8B8A8_UNORM };
	TUniquePtr<D3D12Fence> m_PresentFence;

	void CreateSwapchain();
	void OnResize();
	void UpdateBackBuffer();
};
