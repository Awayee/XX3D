#pragma once
#include "VulkanCommon.h"
#include "VulkanResources.h"

class VulkanSwapchain : public RHISwapChain {
public:
	explicit VulkanSwapchain(const VulkanContext* context);
	~VulkanSwapchain() override;
	bool Present(TConstArrayView<VkSemaphore> smps);
	void Resize(USize2D size) override;
	USize2D GetExtent() override;
private:
	enum : uint32 {
		WAIT_MAX = 0xff,
		MAX_FRAME_COUNT = 3,
	};
	const VulkanContext* m_ContextPtr;
	uint32 m_Width;
	uint32 m_Height;
	VkSurfaceFormatKHR m_SurfaceFormat;
	VkSwapchainKHR m_Swapchain;
	TVector<RHIVkTexture> m_Textures;
	uint8 m_CurFrame{ 0 };
	bool m_Prepared = false;
	struct FrameResource {
		uint32 ImageIdx;
		VkSemaphore ImageAvailableSmp{ VK_NULL_HANDLE };
		VkSemaphore PreparePresentSmp{ VK_NULL_HANDLE };
	};
	TVector<FrameResource> m_FrameRes;
	void CreateSwapChain();
	void ClearSwapChain();
};