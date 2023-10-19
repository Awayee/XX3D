#include "VulkanSwapchain.h"
#include "VulkanBuilder.h"

VulkanSwapchain::VulkanSwapchain(const VulkanContext* context) : m_ContextPtr(context) {
	ASSERT(m_ContextPtr, "");
	CreateSwapChain();

	// Create semaphores
	m_FrameRes.Resize(MAX_FRAME_COUNT);
	VkSemaphoreCreateInfo smpInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr };
	for (FrameResource& res : m_FrameRes) {
		vkCreateSemaphore(m_ContextPtr->Device, &smpInfo, nullptr, &res.ImageAvailableSmp);
		vkCreateSemaphore(m_ContextPtr->Device, &smpInfo, nullptr, &res.PreparePresentSmp);
	}
}

VulkanSwapchain::~VulkanSwapchain() {
	ClearSwapChain();
	for (FrameResource& res : m_FrameRes) {
		vkDestroySemaphore(m_ContextPtr->Device, res.ImageAvailableSmp, nullptr);
		vkDestroySemaphore(m_ContextPtr->Device, res.PreparePresentSmp, nullptr);
	}
}

bool VulkanSwapchain::Present(TConstArrayView<VkSemaphore> smps) {
	if (m_Prepared) {
		VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, nullptr };
		presentInfo.waitSemaphoreCount = smps.Size();
		presentInfo.pWaitSemaphores = smps.Data();
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_Handle;
		presentInfo.pImageIndices = &m_FrameRes[m_CurFrame].ImageIdx;
		vkQueuePresentKHR(m_ContextPtr->PresentQueue.Handle, &presentInfo);
		m_CurFrame = (m_CurFrame + 1) % m_FrameRes.Size();
		m_Prepared = false;
	}
	FrameResource& currentFrame = m_FrameRes[m_CurFrame];
	VkResult res = vkAcquireNextImageKHR(m_ContextPtr->Device, m_Handle, WAIT_MAX, currentFrame.ImageAvailableSmp, VK_NULL_HANDLE, &currentFrame.ImageIdx);
	if (VK_SUCCESS == res) {
		m_Prepared = true;
		return true;
	}
	return false;
}

void VulkanSwapchain::Resize(USize2D size) {
	ClearSwapChain();
	CreateSwapChain();
}

USize2D VulkanSwapchain::GetExtent() {
	return USize2D{ m_Width, m_Height };
}

void VulkanSwapchain::CreateSwapChain() {
	SwapChainBuilder initializer(m_Handle, m_Images, m_Views);
	initializer.Extent = { m_Width, m_Height };
	initializer.SetContext(*m_ContextPtr);
	initializer.Build();
}

void VulkanSwapchain::ClearSwapChain() {
	VkDevice device = m_ContextPtr->Device;
	vkDestroySwapchainKHR(device, m_Handle, nullptr);
	for (VkImage image : m_Images) {
		vkDestroyImage(device, image, nullptr);
	}for (VkImageView view : m_Views) {
		vkDestroyImageView(device, view, nullptr);
	}
	m_Images.Clear();
	m_Views.Clear();
}