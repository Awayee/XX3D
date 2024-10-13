#pragma once
#include "VulkanCommon.h"
#include "VulkanResources.h"
#include "VulkanDevice.h"
#include "RHI/Public/RHI.h"

class VulkanBackBuffer: public VulkanRHITexture {
public:
	VulkanBackBuffer(const RHITextureDesc& desc) : VulkanRHITexture(desc), m_Image(nullptr), m_View(nullptr) {}
	void ResetImage(VkImage image, VkImageView view);
	void UpdateData(const void* data, uint32 byteSize, RHITextureSubRes subRes, IOffset3D offset) override { CHECK(0); }
	VkImageView GetView(RHITextureSubRes subRes) override { return m_View; }
	VkImage GetImage() override { return m_Image; }
private:
	VkImage m_Image;
	VkImageView m_View;
};

class VulkanViewport: public RHIViewport {
public:
	explicit VulkanViewport(const VulkanContext* context, VulkanDevice* device, WindowHandle window, USize2D size, const RHIInitConfig& cfg);
	~VulkanViewport() override;
	void SetSize(USize2D size) override;
	USize2D GetSize() override;
	bool PrepareBackBuffer() override;
	RHITexture* GetBackBuffer() override;
	ERHIFormat GetBackBufferFormat() override;
	void Present() override;
	VkSemaphore GetCurrentSemaphore() const;
	USize2D GetSize() const;
	uint32 GetImageCount() const;
private:
	const VulkanContext* m_Context;
	VulkanDevice* m_Device;
	WindowHandle m_Window;
	USize2D m_Size;
	VkSurfaceKHR m_Surface;
	VkSwapchainKHR m_Swapchain {VK_NULL_HANDLE};
	ERHIFormat m_BackBufferFormat;
	TUniquePtr<VulkanBackBuffer> m_BackBuffer;
	struct VulkanImage {
		VkImage Image;
		VkImageView View;
	};
	TArray<VulkanImage> m_SwapchainImages;
	TStaticArray<VkSemaphore, RHI_FRAME_IN_FLIGHT_MAX> m_Semaphores;
	bool m_EnableVSync{ false };
	bool m_SizeDirty{ false };
	uint32 m_BackBufferIdx{ 0 };
	const VulkanQueue* m_PresentQueue{ nullptr };
	void CreateSwapchain();// create swap chain after size assigned.
	void DestroySwapchain();
};