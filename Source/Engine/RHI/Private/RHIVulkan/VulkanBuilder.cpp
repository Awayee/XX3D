#include "VulkanBuilder.h"
#include "Core/Public/Container.h"
#include <GLFW/glfw3.h>


constexpr uint32 INVALID_INDEX = UINT32_MAX;

struct DeviceInfo {
	uint32 Score{ 0 };
	uint32 m_GraphicsIdx{ INVALID_INDEX };
	uint32 m_PresentIdx{ INVALID_INDEX };
	uint32 ComputeIndex{ INVALID_INDEX };
	uint32 ImageCount{ 0 };
	VkSurfaceFormatKHR SwapchainFormat{ VK_FORMAT_UNDEFINED };
	VkPresentModeKHR SwapchainPresentMode{ VK_PRESENT_MODE_MAX_ENUM_KHR };
};


bool CheckValidationLayerSupport(const TVector<const char*>& validationLayers)
{
	uint32 layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	TVector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.Data());
	for (auto& name : validationLayers)
	{
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(name, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
		{
			return false;
		}
	}
	return true;
}

inline bool CheckExtensionsIsSupported(VkPhysicalDevice device, TVector<const char*> extensions) {
	uint32 extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
	TVector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.Data());
	bool isSupport = false;
	for (const char* extensionName : extensions) {
		bool flag = false;
		for (auto& extension : availableExtensions) {
			if (strcmp(extensionName, extension.extensionName) == 0) {
				flag = true;
				isSupport = true;
				break;
			}
		}
		if (flag) break;
	}
	return isSupport;
}

void FindQueueFamilyIndex(const VkPhysicalDevice& device, const VkSurfaceKHR& surface, uint32* pGraphicsIndex, uint32* pPresentIndex, uint32* pComputeIndex)
{
	*pGraphicsIndex = INVALID_INDEX;
	*pPresentIndex = INVALID_INDEX;
	*pComputeIndex = INVALID_INDEX;
	uint32 queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	TVector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.Data());
	for (uint32 i = 0; i < queueFamilyCount; i++) {
		if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			*pGraphicsIndex = i;
		}
		if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
			*pComputeIndex = i;
		}
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
		if (presentSupport) {
			*pPresentIndex = i;
		}

		if (*pGraphicsIndex != INVALID_INDEX && *pPresentIndex != INVALID_INDEX && *pComputeIndex != INVALID_INDEX) break;
	}
}

inline VkSurfaceFormatKHR CheckSurfaceFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceFormatKHR desiredFormat) {
	uint32 formatCount;
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr), "vkGetPhysicalDeviceSurfaceFormatsKHR");
	TVector<VkSurfaceFormatKHR> formats(formatCount);
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.Data()), "vkGetPhysicalDeviceSurfaceFormatsKHR");
	for (const auto& format : formats) {
		if (format.format == desiredFormat.format && format.colorSpace == desiredFormat.colorSpace) {
			return format;
		}
	}
	XX_ERROR("Could not find swapchain format (%i, %i)", desiredFormat.format, desiredFormat.colorSpace);
	return { VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_MAX_ENUM_KHR };
}

inline VkPresentModeKHR FindPresentMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
	uint32 presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
	TVector<VkPresentModeKHR> presentModes(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.Data());
	if (!presentModes.Empty()) {
		static const TVector<VkPresentModeKHR> s_PresentModePriorities{
			VK_PRESENT_MODE_IMMEDIATE_KHR,
			VK_PRESENT_MODE_MAILBOX_KHR,
			VK_PRESENT_MODE_FIFO_KHR,
			VK_PRESENT_MODE_FIFO_RELAXED_KHR
		};
		TUnorderedMap<VkPresentModeKHR, uint32> enumMap;
		for (uint32 i = 0; i < s_PresentModePriorities.Size(); ++i) {
			enumMap[s_PresentModePriorities[i]] = i;
		}
		uint32 minIdx = UINT32_MAX;
		for (VkPresentModeKHR presentMode : presentModes) {
			auto exist = enumMap.find(presentMode);
			if (exist != enumMap.end() && exist->second < minIdx) {
				minIdx = exist->second;
			}
		}
		if (UINT32_MAX != minIdx) {
			return s_PresentModePriorities[minIdx];
		}
	}
	return VK_PRESENT_MODE_MAX_ENUM_KHR;
}

inline DeviceInfo GetPhysicalDeviceInfo(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, const ContextBuilder::SDesc& desc) {
	DeviceInfo info;
	if (!CheckExtensionsIsSupported(physicalDevice, desc.DeviceExtensions)) {
		return info;
	}
	// query family index
	FindQueueFamilyIndex(physicalDevice, surface, &info.m_GraphicsIdx, &info.m_PresentIdx, &info.ComputeIndex);
	if (INVALID_INDEX == info.m_GraphicsIdx || INVALID_INDEX == info.m_PresentIdx || INVALID_INDEX == info.ComputeIndex) {
		return info;
	}

	// query swapchain info
	// format
	info.SwapchainFormat = CheckSurfaceFormat(physicalDevice, surface, desc.SurfaceFormat);
	//present mode
	info.SwapchainPresentMode = FindPresentMode(physicalDevice, surface);

	if (info.SwapchainFormat.format == VK_FORMAT_UNDEFINED || info.SwapchainPresentMode == VK_PRESENT_MODE_MAX_ENUM_KHR) {
		return info;
	}

	info.Score = 1;

	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(physicalDevice, &properties);

	// discrete gpu is first
	VkPhysicalDeviceType preferredType = desc.IntegratedGPU ? VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU : VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
	if (properties.deviceType == preferredType) {
		info.Score += 100;
	}
	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceFeatures(physicalDevice, &features);
	if (features.geometryShader) {
		info.Score += 10;
	}

	return info;
}

inline void InitDeviceQueue(VkDevice device, VulkanQueue& queue) {
	vkGetDeviceQueue(device, queue.FamilyIndex, queue.QueueIndex, &queue.Handle);
}

inline VkFormat FindDepthFormat(VkPhysicalDevice physicalDevice) {

	const TVector<VkFormat> candidates{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
	VkImageTiling tiling{ VK_IMAGE_TILING_OPTIMAL };
	VkFormatFeatureFlags features{ VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT };
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}
	XX_ERROR("findSupportedFormat failed");
	return VK_FORMAT_UNDEFINED;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT,
	VkDebugUtilsMessageTypeFlagsEXT,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* data)
{
	XX_ERROR("[Vulkan Error] %s", pCallbackData->pMessage);
	return VK_FALSE;
}

inline void SetDebugInfo(VkDebugUtilsMessengerCreateInfoEXT& debugInfo) {
	debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugInfo.pfnUserCallback = DebugCallback;
}

ContextBuilder::ContextBuilder(VulkanContext& context): m_Context(context) {
}

void ContextBuilder::Build() {
	CreateInstance();
	InitializeDebugger();
	CreateWindowSurface();
	PickGPU();
	CreateDevice();
}

void ContextBuilder::Release() {
	if(!m_Context.Instance) {
		return;
	}
	if(m_Context.DebugMsg) {
		vkDestroyDebugUtilsMessengerEXT(m_Context.Instance, m_Context.DebugMsg, nullptr);
	}
	vkDestroyDevice(m_Context.Device, nullptr);
	vkDestroyInstance(m_Context.Instance, nullptr);
	m_Context = {};
}

TVector<const char*> ContextBuilder::GetInstanceExtensions() {
	uint32     glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	TVector<const char*> extensions(glfwExtensionCount);
	memcpy(extensions.Data(), glfwExtensions, glfwExtensionCount * sizeof(const char*));
	if(Desc.EnableDebug){
		extensions.PushBack(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	return extensions;
}

void ContextBuilder::CreateInstance() {
	InitializeInstanceProcAddr();
	if(Desc.EnableDebug) {
		ASSERT(CheckValidationLayerSupport(Desc.ValidationLayers), "validation layers requested, but not available!");
	}
	// app info
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = Desc.AppName.c_str();
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = Desc.AppName.c_str();
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_2;

	// create info
	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo; // the appInfo is stored here

	auto extensions = GetInstanceExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32>(extensions.Size());
	createInfo.ppEnabledExtensionNames = extensions.Data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (Desc.EnableDebug) {
		createInfo.enabledLayerCount = static_cast<uint32>(Desc.ValidationLayers.Size());
		createInfo.ppEnabledLayerNames = Desc.ValidationLayers.Data();
		SetDebugInfo(debugCreateInfo);
		debugCreateInfo.pUserData = (void*)this;
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	VK_CHECK(vkCreateInstance(&createInfo, nullptr, &m_Context.Instance), "");
	vkEnumerateInstanceVersion(&m_Context.APIVersion);
	LoadInstanceFunctions(m_Context.Instance);
}

void ContextBuilder::InitializeDebugger() {
	if(Desc.EnableDebug) {
		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		SetDebugInfo(createInfo);
		createInfo.pUserData = (void*)this;
		VK_CHECK(vkCreateDebugUtilsMessengerEXT(m_Context.Instance, &createInfo, nullptr, &m_Context.DebugMsg), "vk create debug messenger!");
	}
}

void ContextBuilder::CreateWindowSurface() {
	m_Context.WindowHandle = Desc.WindowHandle;
	VK_CHECK(glfwCreateWindowSurface(m_Context.Instance, static_cast<GLFWwindow*>(Desc.WindowHandle), nullptr, &m_Context.Surface), "vk create window surface");
}

void ContextBuilder::PickGPU() {
	uint32 deviceCount = 0;
	VK_CHECK(vkEnumeratePhysicalDevices(m_Context.Instance, &deviceCount, nullptr), "no avaliable physical device");
	TVector<VkPhysicalDevice> devices(deviceCount);
	VK_CHECK(vkEnumeratePhysicalDevices(m_Context.Instance, &deviceCount, devices.Data()), "no avaliable physical device");
	TVector<TPair<VkPhysicalDevice, DeviceInfo>> devicesInfo;
	for (auto& device : devices) {
		auto deviceInfo = GetPhysicalDeviceInfo(device, m_Context.Surface, Desc);
		if (deviceInfo.Score > 0) {
			devicesInfo.PushBack({ device, std::move(deviceInfo) });
		}
	}
	if(devicesInfo.Empty()) {
		XX_ERROR("Could not pick a GPU!");
	}
	devicesInfo.Sort([](const TPair<VkPhysicalDevice, DeviceInfo> p1, const TPair<VkPhysicalDevice, DeviceInfo> p2) {return p1.second.Score > p2.second.Score; });
	m_Context.PhysicalDevice = devicesInfo[0].first;
	auto& deviceInfo = devicesInfo[0].second;
	vkGetPhysicalDeviceProperties(m_Context.PhysicalDevice, &m_Context.DeviceProperties);
	PRINT("Picked GPU: %s %u", m_Context.DeviceProperties.deviceName, m_Context.DeviceProperties.deviceID);
	m_Context.GraphicsQueue.FamilyIndex = deviceInfo.m_GraphicsIdx;
	m_Context.PresentQueue.FamilyIndex = deviceInfo.m_PresentIdx;
	m_Context.ComputeQueue.FamilyIndex = deviceInfo.ComputeIndex;
	m_Context.SwapchainFormat =deviceInfo.SwapchainFormat;//record
}

void ContextBuilder::CreateDevice() {
	TUnorderedSet<uint32> queueFamilies{ m_Context.GraphicsQueue.FamilyIndex, m_Context.PresentQueue.FamilyIndex, m_Context.ComputeQueue.FamilyIndex };
	TVector<VkDeviceQueueCreateInfo> queueCreateInfos;
	float queuePriority = 1.0f;
	for (uint32 queueFamily : queueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.PushBack(std::move(queueCreateInfo));
	}

	VkPhysicalDeviceFeatures features{};
	features.samplerAnisotropy = VK_TRUE;
	features.fragmentStoresAndAtomics = VK_TRUE;
	features.independentBlend = VK_TRUE;
	features.geometryShader = VK_TRUE;

	VkDeviceCreateInfo deviceCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.Data();
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32>(queueCreateInfos.Size());
	deviceCreateInfo.pEnabledFeatures = &features;
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32>(Desc.DeviceExtensions.Size());
	deviceCreateInfo.ppEnabledExtensionNames = Desc.DeviceExtensions.Data();
	deviceCreateInfo.enabledLayerCount = 0;

	VK_CHECK(vkCreateDevice(m_Context.PhysicalDevice, &deviceCreateInfo, nullptr, &m_Context.Device), "vkCreateDevice");

	// initialize queues of this device
	InitDeviceQueue(m_Context.Device, m_Context.GraphicsQueue);
	InitDeviceQueue(m_Context.Device, m_Context.PresentQueue);
	InitDeviceQueue(m_Context.Device, m_Context.ComputeQueue);

	m_Context.DepthFormat = FindDepthFormat(m_Context.PhysicalDevice);
}

SwapChainBuilder::SDesc::SDesc(const VulkanContext& context) {
	PhysicalDevice = context.PhysicalDevice;
	Surface = context.Surface;
	Device = context.Device;
	GraphicsIdx = context.GraphicsQueue.FamilyIndex;
	PresentIdx = context.PresentQueue.FamilyIndex;
	SwapchainFormat = context.SwapchainFormat;
}


SwapChainBuilder::SwapChainBuilder(VkSwapchainKHR& handle, TVector<VkImage>& images, TVector<VkImageView>& imageViews):
m_Handle(handle), m_Images(images), m_Views(imageViews) {}

void SwapChainBuilder::Build() {
	//extent
	VkSurfaceCapabilitiesKHR capabilities;
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Desc.PhysicalDevice, Desc.Surface, &capabilities), "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
	if(0 == Desc.Extent.width || 0 == Desc.Extent.height) {
		if (capabilities.currentExtent.width != UINT32_MAX) {
			Desc.Extent = capabilities.currentExtent;
		}
		else {
			Desc.Extent = capabilities.maxImageExtent;
		}
	}

	uint32 imageCount = capabilities.minImageCount + 1;
	if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
		imageCount = capabilities.maxImageCount;
	}
	VkSurfaceFormatKHR surfaceFormat = CheckSurfaceFormat(Desc.PhysicalDevice, Desc.Surface, Desc.SwapchainFormat);
	VkSwapchainCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR, nullptr };
	createInfo.surface = Desc.Surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = Desc.Extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	if (Desc.GraphicsIdx != Desc.PresentIdx)
	{
		uint32 queueFamilyIndices[] = { Desc.GraphicsIdx, Desc.PresentIdx };
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	createInfo.preTransform = capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = FindPresentMode(Desc.PhysicalDevice, Desc.Surface);
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;
	VK_CHECK(vkCreateSwapchainKHR(Desc.Device, &createInfo, nullptr, &m_Handle), "CreateSwapchain");

	// swapchain images
	vkGetSwapchainImagesKHR(Desc.Device, m_Handle, &imageCount, nullptr);
	m_Images.Resize(imageCount);
	vkGetSwapchainImagesKHR(Desc.Device, m_Handle, &imageCount, m_Images.Data());
	m_Views.Resize(imageCount);

	// create imageview (one for each this time) for all swapchain images
	for (uint64 i = 0; i < m_Images.Size(); i++)
	{
		VkImageViewCreateInfo imageViewCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, nullptr, 0 };
		imageViewCreateInfo.image = m_Images[i];
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = surfaceFormat.format;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;
		VK_CHECK(vkCreateImageView(Desc.Device, &imageViewCreateInfo, nullptr, &m_Views[i]), "Create SwapChain, ImageView");
	}
}
