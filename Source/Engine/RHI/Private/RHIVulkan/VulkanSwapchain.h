#pragma once
#include "VulkanCommon.h"
#include "VulkanResources.h"
#include "Core/Public/TSingleton.h"

class VulkanSwapchain: public TSingleton<VulkanSwapchain> {
public:
	VkSemaphore AcquireImage();// acquire an available image, return VK_NULL_HANDLE if failed
	bool Present(VkSemaphore waitSmp);// return false if failed
	void Resize(USize2D size);
	USize2D GetSize() const;
	uint32 GetImageCount() const;
private:
	friend TSingleton<VulkanSwapchain>;
	enum : uint32 {
		WAIT_MAX = 0xff,
		MAX_FRAME_COUNT = 3,
	};
	const VulkanContext* m_ContextPtr;
	uint32 m_Width;
	uint32 m_Height;
	VkSurfaceFormatKHR m_SurfaceFormat;
	VkSwapchainKHR m_Swapchain;
	TVector<RHIVulkanTexture> m_Textures;
	uint8 m_CurFrame{ 0 };
	bool m_Prepared = false;
	struct FrameImage {
		uint32 ImageIdx;
		VkSemaphore ImageAvailableSmp{ VK_NULL_HANDLE };
	};
	TVector<FrameImage> m_FrameRes;
	explicit VulkanSwapchain(const VulkanContext* context);
	~VulkanSwapchain();
	void CreateSwapChain();
	void ClearSwapChain();
};