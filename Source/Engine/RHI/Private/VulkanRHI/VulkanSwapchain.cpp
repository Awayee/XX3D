#include "VulkanSwapchain.h"
#include "Math/Public/Math.h"
#include <GLFW/glfw3.h>

inline VkSurfaceFormatKHR ChooseSurfaceFormat(const VkSurfaceFormatKHR* data, uint32 count) {
	static VkSurfaceFormatKHR s_PreferredFormat{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	ASSERT(0 != count, "No surface format available!");
	if(1 == count && data[0].format == VK_FORMAT_UNDEFINED) {
		return  s_PreferredFormat;
	}
	for(uint32 i=0; i<count; ++i) {
		if(data[i].format == s_PreferredFormat.format && data[i].colorSpace == s_PreferredFormat.colorSpace) {
			return s_PreferredFormat;
		}
	}
	return data[0];
}

inline ERHIFormat SurfaceFormatToRHIFormat(VkFormat f) {
	// only support preferred formats
	if(VK_FORMAT_B8G8R8A8_UNORM == f) {
		return ERHIFormat::B8G8R8A8_UNORM;
	}
	return ERHIFormat::Undefined;
}

inline VkPresentModeKHR ChoosePresentMode(const VkPresentModeKHR* data, uint32 count) {
	VkPresentModeKHR s_PreferredMode[] { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR };
	for(const VkPresentModeKHR presentMode : s_PreferredMode) {
		for(uint32 i=0; i<count; ++i) {
			if(data[i] == presentMode) {
				return presentMode;
			}
		}
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

inline VkExtent2D GetSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities, USize2D desiredSize) {
	if (capabilities.currentExtent.width !=  UINT32_MAX) {
		return capabilities.currentExtent;
	}
	return {
		Math::Clamp<uint32>(desiredSize.w, capabilities.minImageExtent.width, capabilities.maxImageExtent.height),
		Math::Clamp<uint32>(desiredSize.h, capabilities.maxImageExtent.width, capabilities.maxImageExtent.height)
	};
}

VulkanSwapchain::VulkanSwapchain(const VulkanContext* context, VulkanDevice* device, WindowHandle window, USize2D size) :
	m_Context(context),
	m_Device(device),
	m_Window(window),
	m_Size(size),
	m_Surface(VK_NULL_HANDLE){
	// Create vulkan surface
	VK_ASSERT(glfwCreateWindowSurface(m_Context->GetInstance(), static_cast<GLFWwindow*>(m_Window), nullptr, &m_Surface), "vk create window surface");
	// Get present queue
	m_PresentQueue = m_Device->FindPresentQueue(m_Surface);
	ASSERT(m_PresentQueue, "[VulkanSwapchain]Could not find a present queue!");
	// Create Swapchain
	CreateSwapchain();
	// Create semaphores
	const uint32 frameCount = Math::Max<uint32>(MAX_FRAME_COUNT, GetImageCount());
	m_Semaphores.Resize(frameCount);
	VkSemaphoreCreateInfo smpInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
	for (auto& smp : m_Semaphores) {
		vkCreateSemaphore(m_Device->GetDevice(), &smpInfo, nullptr, &smp);
	}
}

VulkanSwapchain::~VulkanSwapchain() {
	DestroySwapchain();
	for (auto& smp : m_Semaphores) {
		vkDestroySemaphore(m_Device->GetDevice(), smp, nullptr);
	}
}

VkSemaphore VulkanSwapchain::AcquireImage() {
	// check size changed before acquire image
	if(m_SizeDirty) {
		DestroySwapchain();
		CreateSwapchain();
		m_SizeDirty = false;
	}

	VkSemaphore signalSmp = m_Semaphores[m_CurFrame];
	if (VK_SUCCESS == vkAcquireNextImageKHR(m_Device->GetDevice(), m_Swapchain, WAIT_MAX, signalSmp, VK_NULL_HANDLE, &m_ImageIdx)) {
		m_Prepared = true;
		m_BackBuffer->UpdateImage(m_Images[m_ImageIdx], m_ImageViews[m_ImageIdx]);
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
		presentInfo.pImageIndices = &m_ImageIdx;
		bRes = VK_SUCCESS == vkQueuePresentKHR(m_PresentQueue->Handle, &presentInfo);
		m_CurFrame = (m_CurFrame + 1) % m_Semaphores.Size();
		m_Prepared = false;
	}
	else {
		LOG_DEBUG("VulkanSwapchain::Present image is not available!");
	}
	return bRes;
}

void VulkanSwapchain::Resize(USize2D size) {
	m_Size = size;
	m_SizeDirty = true;
}

USize2D VulkanSwapchain::GetSize() const {
	return m_Size;
}

uint32 VulkanSwapchain::GetImageCount() const {
	return m_Images.Size();
}

VkFormat VulkanSwapchain::GetSwapchainFormat() const {
	return m_SurfaceFormat.format;
}

void VulkanSwapchain::CreateSwapchain() {
	// create swap chain

	VkPhysicalDevice physicalDevice = m_Device->GetPhysicalDevice();
	//  get capabilities
	VkSurfaceCapabilitiesKHR capabilities{};
	VkSurfaceFormatKHR surfaceFormat{};
	VkPresentModeKHR presentMode{ VK_PRESENT_MODE_FIFO_KHR };
	uint32 imageCount;
	VkExtent2D swapchainExtent;

	VK_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_Surface, &capabilities), "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
	swapchainExtent = GetSwapchainExtent(capabilities, m_Size);
	imageCount = capabilities.minImageCount + 1;
	if (0 != capabilities.maxImageCount && imageCount > capabilities.maxImageCount) {
		imageCount = capabilities.maxImageCount;
	}

	// get formats
	uint32 formatCount;
	VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &formatCount, nullptr), "vkGetPhysicalDeviceSurfaceFormatsKHR");
	if(0 != formatCount) {
		TFixedArray<VkSurfaceFormatKHR> formats(formatCount);
		VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &formatCount, formats.Data()), "vkGetPhysicalDeviceSurfaceFormatsKHR");
		surfaceFormat = ChooseSurfaceFormat(formats.Data(), formatCount);
	}

	// get present mode
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_Surface, &presentModeCount, nullptr);
	if (presentModeCount != 0) {
		TFixedArray<VkPresentModeKHR> presentModes(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_Surface, &presentModeCount, presentModes.Data());
		presentMode = ChoosePresentMode(presentModes.Data(), presentModeCount);
	}

	VkSwapchainCreateInfoKHR swapchainInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR, nullptr, 0 };
	swapchainInfo.surface = m_Surface;
	swapchainInfo.minImageCount = imageCount;
	swapchainInfo.imageFormat = surfaceFormat.format;
	swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapchainInfo.imageExtent = swapchainExtent;
	swapchainInfo.imageArrayLayers = 1;
	swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainInfo.queueFamilyIndexCount = 0;
	swapchainInfo.pQueueFamilyIndices = nullptr;
	swapchainInfo.preTransform = capabilities.currentTransform;
	swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainInfo.presentMode = presentMode;
	swapchainInfo.clipped = VK_TRUE;
	swapchainInfo.oldSwapchain = VK_NULL_HANDLE;
	vkCreateSwapchainKHR(m_Device->GetDevice(), &swapchainInfo, nullptr, &m_Swapchain);

	// get swap chain images
	uint32 swapchainImageCount; 
	vkGetSwapchainImagesKHR(m_Device->GetDevice(), m_Swapchain, &swapchainImageCount, nullptr);
	m_Images.Resize(swapchainImageCount);
	vkGetSwapchainImagesKHR(m_Device->GetDevice(), m_Swapchain, &swapchainImageCount, m_Images.Data());
	m_ImageViews.Resize(swapchainImageCount);
	for(uint32 i=0; i<swapchainImageCount; ++i) {
		VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, nullptr, 0 };
		viewInfo.image = m_Images[i];
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = surfaceFormat.format;
		viewInfo.components.r = viewInfo.components.g = viewInfo.components.b = viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;
		VK_ASSERT(vkCreateImageView(m_Device->GetDevice(), &viewInfo, nullptr, &m_ImageViews[i]), "");
	}

	// create back buffer
	RHITextureDesc backBufferDesc;
	backBufferDesc.Dimension = ETextureDimension::Tex2D;
	backBufferDesc.Format = SurfaceFormatToRHIFormat(surfaceFormat.format);
	backBufferDesc.Flags = TEXTURE_FLAG_PRESENT;
	backBufferDesc.Size = { swapchainExtent.width, swapchainExtent.height };
	backBufferDesc.Depth = 1;
	backBufferDesc.ArraySize = 1;
	backBufferDesc.NumMips = 1;
	backBufferDesc.Samples = 1;
	m_BackBuffer.Reset(new VulkanBackBuffer(backBufferDesc, m_Device));

	m_SurfaceFormat = surfaceFormat;
}

void VulkanSwapchain::DestroySwapchain() {
	for(VkImageView view: m_ImageViews) {
		vkDestroyImageView(m_Device->GetDevice(), view, nullptr);
	}
	m_ImageViews.Clear();
	for(VkImage image: m_Images) {
		vkDestroyImage(m_Device->GetDevice(), image, nullptr);
	}
	m_Images.Clear();

	m_BackBuffer.Reset();
	vkDestroySwapchainKHR(m_Device->GetDevice(), m_Swapchain, nullptr);
}