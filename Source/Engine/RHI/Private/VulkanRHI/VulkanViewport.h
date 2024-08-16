#pragma once
#include "VulkanCommon.h"
#include "VulkanResources.h"
#include "VulkanDevice.h"

class VulkanBackBuffer: public VulkanRHITexture {
public:
	VulkanBackBuffer(const RHITextureDesc& desc, VulkanDevice* device, VkImage image);
};

class VulkanViewport: public RHIViewport {
public:
	explicit VulkanViewport(const VulkanContext* context, VulkanDevice* device, WindowHandle window, USize2D size);
	~VulkanViewport() override;
	void SetSize(USize2D size) override;
	USize2D GetSize() override;
	RHITexture* AcquireBackBuffer() override;
	RHITexture* GetCurrentBackBuffer() override;
	void Present() override;
	VkSemaphore GetCurrentSemaphore() const;
	USize2D GetSize() const;
	uint32 GetImageCount() const;
	VkFormat GetSwapchainFormat() const;
private:
	const VulkanContext* m_Context;
	VulkanDevice* m_Device;
	WindowHandle m_Window;
	USize2D m_Size;
	VkSurfaceKHR m_Surface;
	VkSwapchainKHR m_Swapchain {VK_NULL_HANDLE};
	VkSurfaceFormatKHR m_SurfaceFormat;
	TArray<TUniquePtr<VulkanBackBuffer>> m_BackBuffers;
	TStaticArray<VkSemaphore, RHI_MAX_FRAME_IN_FLIGHT> m_Semaphores;
	bool m_SizeDirty{ false };
	uint32 m_BackBufferIdx{ 0 };
	const VulkanQueue* m_PresentQueue{ nullptr };
	void CreateSwapchain();// create swap chain after size assigned.
	void DestroySwapchain();
};