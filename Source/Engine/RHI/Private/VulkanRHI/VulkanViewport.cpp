#include "VulkanViewport.h"
#include "VulkanCommand.h"
#include "VulkanConverter.h"
#include "Math/Public/Math.h"
#include "System/Public/FrameCounter.h"
#include <GLFW/glfw3.h>

inline VkSurfaceFormatKHR ChooseSurfaceFormat(const VkSurfaceFormatKHR* data, uint32 count) {
	static VkSurfaceFormatKHR s_PreferredFormat{ VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
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
	if(VK_FORMAT_R8G8B8A8_UNORM == f) {
		return ERHIFormat::R8G8B8A8_UNORM;
	}
	return ERHIFormat::Undefined;
}

inline VkPresentModeKHR ChoosePresentMode(const VkPresentModeKHR* data, uint32 count, bool enableVSync) {
	const VkPresentModeKHR preferred = enableVSync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
	for (uint32 i = 0; i < count; ++i) {
		if (data[i] == preferred) {
			return preferred;
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

void VulkanBackBuffer::ResetImage(VkImage image, VkImageView view) {
	m_Image = image;
	m_View = view;
}

VulkanViewport::VulkanViewport(const VulkanContext* context, VulkanDevice* device, WindowHandle window, USize2D size, const RHIInitConfig& cfg) :
	m_Context(context),
	m_Device(device),
	m_Window(window),
	m_Size(size),
	m_Surface(VK_NULL_HANDLE),
	m_EnableVSync(cfg.EnableVSync){
	// Create vulkan surface
	VK_ASSERT(glfwCreateWindowSurface(m_Context->GetInstance(), static_cast<GLFWwindow*>(m_Window), nullptr, &m_Surface), "vk create window surface");
	// Get present queue
	m_PresentQueue = m_Device->FindPresentQueue(m_Surface);
	ASSERT(m_PresentQueue, "[VulkanViewport]Could not find a present queue!");
	// Create Swapchain
	CreateSwapchain();
	// Create semaphores for per frame
	VkSemaphoreCreateInfo smpInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
	for (uint32 i = 0; i < RHI_FRAME_IN_FLIGHT_MAX; ++i) {
		vkCreateSemaphore(m_Device->GetDevice(), &smpInfo, nullptr, &m_Semaphores[i]);
	}
}

VulkanViewport::~VulkanViewport() {
	DestroySwapchain();
	for(VkSemaphore smp: m_Semaphores) {
		vkDestroySemaphore(m_Device->GetDevice(), smp, nullptr);
	}
	vkDestroySurfaceKHR(m_Context->GetInstance(), m_Surface, nullptr);
}

void VulkanViewport::SetSize(USize2D size) {
	m_SizeDirty = true;
	m_Size = size;
}

USize2D VulkanViewport::GetSize() {
	return m_Size;
}

bool VulkanViewport::PrepareBackBuffer() {
	// check size changed before acquire image
	if (m_SizeDirty) {
		DestroySwapchain();
		CreateSwapchain();
		m_SizeDirty = false;
	}

	if(!m_Swapchain) {
		return false;
	}

	if (VK_SUCCESS == vkAcquireNextImageKHR(m_Device->GetDevice(), m_Swapchain, VK_WAIT_MAX, GetCurrentSemaphore(), VK_NULL_HANDLE, &m_BackBufferIdx)) {
		VulkanImage& vulkanImage = m_SwapchainImages[m_BackBufferIdx];
		m_BackBuffer->ResetImage(vulkanImage.Image, vulkanImage.View);
	}
	return true;
}

RHITexture* VulkanViewport::GetBackBuffer() {
	return m_BackBuffer.Get();
}

ERHIFormat VulkanViewport::GetBackBufferFormat() {
	return m_BackBufferFormat;
}

void VulkanViewport::Present() {
	TArray<VkSemaphore> waitSemaphores;
	m_Device->GetCommandContext()->GetLastSubmissionSemaphores(waitSemaphores);
	VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, nullptr };
	presentInfo.waitSemaphoreCount = waitSemaphores.Size();
	presentInfo.pWaitSemaphores = waitSemaphores.Data();
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_Swapchain;
	presentInfo.pImageIndices = &m_BackBufferIdx;
	vkQueuePresentKHR(m_PresentQueue->Handle, &presentInfo);
}

VkSemaphore VulkanViewport::GetCurrentSemaphore() const {
	return m_Semaphores[FrameCounter::GetFrame() % RHI_FRAME_IN_FLIGHT_MAX];
}

USize2D VulkanViewport::GetSize() const {
	return m_Size;
}

uint32 VulkanViewport::GetImageCount() const {
	return m_SwapchainImages.Size();
}

void VulkanViewport::CreateSwapchain() {
	// Window Size may be 0 if window is minimized
	if(m_Size.w == 0 || m_Size.h == 0) {
		return;
	}
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
		presentMode = ChoosePresentMode(presentModes.Data(), presentModeCount, m_EnableVSync);
	}

	VkSwapchainCreateInfoKHR swapchainInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR, nullptr, 0 };
	swapchainInfo.surface = m_Surface;
	swapchainInfo.minImageCount = imageCount;
	swapchainInfo.imageFormat = surfaceFormat.format;
	swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapchainInfo.imageExtent = swapchainExtent;
	swapchainInfo.imageArrayLayers = 1;
	swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainInfo.queueFamilyIndexCount = 0;
	swapchainInfo.pQueueFamilyIndices = nullptr;
	swapchainInfo.preTransform = capabilities.currentTransform;
	swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainInfo.presentMode = presentMode;
	swapchainInfo.clipped = VK_TRUE;
	swapchainInfo.oldSwapchain = VK_NULL_HANDLE;
	vkCreateSwapchainKHR(m_Device->GetDevice(), &swapchainInfo, nullptr, &m_Swapchain);

	// Get swap chain images
	uint32 swapchainImageCount; 
	vkGetSwapchainImagesKHR(m_Device->GetDevice(), m_Swapchain, &swapchainImageCount, nullptr);
	TFixedArray<VkImage> swapchainImages(swapchainImageCount);
	vkGetSwapchainImagesKHR(m_Device->GetDevice(), m_Swapchain, &swapchainImageCount, swapchainImages.Data());
	LOG_DEBUG("Image count of swapchain is %u.", swapchainImageCount);
	// Create back buffers
	m_BackBufferFormat = SurfaceFormatToRHIFormat(surfaceFormat.format);
	RHITextureDesc backBufferDesc = RHITextureDesc::Texture2D();
	backBufferDesc.Format = m_BackBufferFormat;
	backBufferDesc.Flags = ETextureFlags::ColorTarget | ETextureFlags::Present;
	backBufferDesc.Width = swapchainExtent.width;
	backBufferDesc.Height = swapchainExtent.height;
	m_BackBuffer.Reset(new VulkanBackBuffer(backBufferDesc));
	// Create swapchain images and views
	m_SwapchainImages.Resize(swapchainImageCount);
	for(uint32 i=0; i<swapchainImageCount; ++i) {
		m_SwapchainImages[i].Image = swapchainImages[i];
		VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, nullptr, 0 };
		viewInfo.image = swapchainImages[i];
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = surfaceFormat.format;
		ToImageSubResourceRange(backBufferDesc.GetDefaultSubRes(), viewInfo.subresourceRange);
		vkCreateImageView(m_Device->GetDevice(), &viewInfo, nullptr, &m_SwapchainImages[i].View);
	}
}

void VulkanViewport::DestroySwapchain() {
	for (auto& vulkanImage : m_SwapchainImages) {
		vkDestroyImageView(m_Device->GetDevice(), vulkanImage.View, nullptr);
	}
	m_SwapchainImages.Reset();
	vkDestroySwapchainKHR(m_Device->GetDevice(), m_Swapchain, nullptr);
	m_Swapchain = VK_NULL_HANDLE;
	m_BackBuffer.Reset();
	
}