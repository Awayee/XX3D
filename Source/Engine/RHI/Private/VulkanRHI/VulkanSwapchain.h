#pragma once
#include "VulkanCommon.h"
#include "VulkanResources.h"
#include "VulkanDevice.h"

class VulkanBackBuffer: public VulkanRHITexture {
public:
	VulkanBackBuffer(const RHITextureDesc& desc, VulkanDevice* device, VkImage image): VulkanRHITexture(desc, device, image, {}){}
};

class VulkanSwapchain {
public:
	explicit VulkanSwapchain(const VulkanContext* context, VulkanDevice* device, WindowHandle window, USize2D size);
	~VulkanSwapchain();
	VkSemaphore AcquireImage();// acquire an available image, return VK_NULL_HANDLE if failed
	bool Present(VkSemaphore waitSmp);// return false if failed
	void Resize(USize2D size);
	USize2D GetSize() const;
	uint32 GetImageCount() const;
	VkFormat GetSwapchainFormat() const;
private:
	enum : uint32 {
		WAIT_MAX = 0xff,
		MAX_FRAME_COUNT = 3,
	};
	const VulkanContext* m_Context;
	VulkanDevice* m_Device;
	WindowHandle m_Window;
	USize2D m_Size;
	VkSurfaceKHR m_Surface;
	VkSwapchainKHR m_Swapchain {VK_NULL_HANDLE};
	VkSurfaceFormatKHR m_SurfaceFormat;
	TVector<TUniquePtr<VulkanBackBuffer>> m_BackBuffers;
	uint32 m_CurFrame{ 0 };
	bool m_Prepared{ false };
	bool m_SizeDirty{ false };
	uint32 m_ImageIdx;
	TVector<VkSemaphore> m_Semaphores;
	const VulkanQueue* m_PresentQueue{ nullptr };
	void CreateSwapchain();// create swap chain after size assigned.
	void DestroySwapchain();
};