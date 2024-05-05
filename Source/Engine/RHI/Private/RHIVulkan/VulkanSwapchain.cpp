#include "VulkanSwapchain.h"
#include "VulkanBuilder.h"

VkSemaphore VulkanSwapchain::AcquireImage() {
	FrameImage& currentFrame = m_FrameRes[m_CurFrame];
	VkSemaphore signalSmp = currentFrame.ImageAvailableSmp;
	VkResult res = vkAcquireNextImageKHR(m_ContextPtr->Device, m_Swapchain, WAIT_MAX, signalSmp, VK_NULL_HANDLE, &currentFrame.ImageIdx);
	if (VK_SUCCESS == res) {
		m_Prepared = true;
		return signalSmp;
	}
	return VK_NULL_HANDLE;
}

bool VulkanSwapchain::Present(VkSemaphore waitSmp) {
	bool bRes = false;
	if (m_Prepared) {
		VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, nullptr };
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &waitSmp;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_Swapchain;
		presentInfo.pImageIndices = &m_FrameRes[m_CurFrame].ImageIdx;
		bRes = VK_SUCCESS == vkQueuePresentKHR(m_ContextPtr->PresentQueue.Handle, &presentInfo);
		m_CurFrame = (m_CurFrame + 1) % m_FrameRes.Size();
		m_Prepared = false;
	}
	else {
		PRINT_DEBUG("VulkanSwapchain::Present image is not available!");
	}
	return bRes;
}

void VulkanSwapchain::Resize(USize2D size) {
	ClearSwapChain();
	CreateSwapChain();
}

USize2D VulkanSwapchain::GetSize() const {
	return USize2D{ m_Width, m_Height };
}

uint32 VulkanSwapchain::GetImageCount() const {
	return m_Textures.Size();
}

VulkanSwapchain::VulkanSwapchain(const VulkanContext* context) : m_ContextPtr(context) {
	ASSERT(m_ContextPtr, "");
	CreateSwapChain();

	// Create semaphores
	m_FrameRes.Resize(MAX_FRAME_COUNT);
	VkSemaphoreCreateInfo smpInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr };
	for (FrameImage& res : m_FrameRes) {
		vkCreateSemaphore(m_ContextPtr->Device, &smpInfo, nullptr, &res.ImageAvailableSmp);
	}
}

VulkanSwapchain::~VulkanSwapchain() {
	ClearSwapChain();
	for (FrameImage& res : m_FrameRes) {
		vkDestroySemaphore(m_ContextPtr->Device, res.ImageAvailableSmp, nullptr);
	}
}

void VulkanSwapchain::CreateSwapChain() {
	TVector<VkImage> images; TVector<VkImageView> views;
	SwapChainBuilder initializer(m_Swapchain, images, views);
	initializer.Desc = SwapChainBuilder::SDesc(*m_ContextPtr);
	initializer.Desc.Extent = { m_Width, m_Height };
	initializer.Build();

	// create swapchain textures
	RHITextureDesc desc;
	desc.Dimension = ETextureDimension::Tex2D;
	desc.Format = ERHIFormat::B8G8R8A8_SRGB;
	desc.Flags = TEXTURE_FLAG_PRESENT;
	desc.Size = { m_Width, m_Height, 1 };
	desc.ArraySize = 1;
	desc.NumMips = 1;
	desc.Samples = 1;
	m_Textures.Reserve(images.Size());
	for(uint32 i=0; i<images.Size(); ++i) {
		m_Textures.PushBack(RHIVulkanTexture{ desc, images[i], views[i] });
	}
}

void VulkanSwapchain::ClearSwapChain() {
	m_Textures.Clear();
	vkDestroySwapchainKHR(m_ContextPtr->Device, m_Swapchain, nullptr);
}