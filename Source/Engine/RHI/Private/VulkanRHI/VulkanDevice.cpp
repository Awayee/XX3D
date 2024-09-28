#include "VulkanDevice.h"
#include "Core/Public/TArray.h"
#include "Core/Public/Container.h"
#include "VulkanMemory.h"
#include "VulkanPipeline.h"
#include "VulkanCommand.h"

inline TArray<const char*> GetDeviceExtensions(const VulkanContext* context) {
	TArray<const char*> extensions;
	extensions.PushBack(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	const uint32 APIVersion = context->GetAPIVersion();
	if(APIVersion < VK_API_VERSION_1_3) {
		extensions.PushBack(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
	}
	return extensions;
}

inline VkFormat FindDepthFormat(VkPhysicalDevice physicalDevice) {
	// find depth format
	const TArray<VkFormat> candidates{ VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT };
	VkImageTiling tiling{ VK_IMAGE_TILING_OPTIMAL };
	VkFormatFeatureFlags features{ VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT };
	for (VkFormat format : candidates){
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features){
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features){
			return format;
		}
	}
	LOG_ERROR("findSupportedFormat failed");
	return VK_FORMAT_UNDEFINED;
}

VulkanDevice::VulkanDevice(const VulkanContext* context, VkPhysicalDevice physicalDevice): m_PhysicalDevice(physicalDevice) {
	// Get device properties
	InitializeDeviceInfo();
	// Create logical device
	CreateDevice(context);
	// Create descriptor manager
	m_DescriptorMgr.Reset(new VulkanDescriptorSetMgr(this));
	// Create memory manager
	m_MemoryAllocator.Reset(new VulkanMemoryAllocator(context, this));
	m_DynamicBufferAllocator.Reset(new VulkanDynamicBufferAllocator(m_MemoryAllocator, this, RHI_DYNAMIC_BUFFER_PAGE));
	// Create command manager
	m_CommandContext.Reset(new VulkanCommandContext(this));
	// Create uploader
	m_Uploader.Reset(new VulkanUploader(this));
}

VulkanDevice::~VulkanDevice() {
	m_Uploader.Reset();
	m_CommandContext.Reset();
	m_DescriptorMgr.Reset();
	m_DynamicBufferAllocator.Reset();
	m_MemoryAllocator.Reset();
	vkDestroyDevice(m_Device, nullptr);
}

const VulkanQueue* VulkanDevice::FindPresentQueue(VkSurfaceKHR surface) const {
	VkBool32 isSupport = VK_FALSE;
	const VulkanQueue& graphicsQueue = GetQueue(EQueueType::Graphics);
	vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, graphicsQueue.FamilyIndex, surface, &isSupport);
	if(isSupport) {
		return &graphicsQueue;
	}
	const VulkanQueue& computeQueue = GetQueue(EQueueType::Compute);
	vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, computeQueue.FamilyIndex, surface, &isSupport);
	if(isSupport) {
		return &computeQueue;
	}
	return nullptr;
}

void VulkanDevice::InitializeDeviceInfo() {
	vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_DeviceProperties);
	m_Formats.DepthFormat = FindDepthFormat(m_PhysicalDevice);
}

void VulkanDevice::CreateDevice(const VulkanContext* context) {
	// Get queue
	uint32 queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);
	TFixedArray<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, queueFamilyProperties.Data());

	// get queue indices
	// the present queue will be created after swapchain
	uint32 graphicsQueueFamilyIdx{ VK_INVALID_INDEX }, computeQueueFamilyIdx{ VK_INVALID_INDEX }, transferQueueFamilyIdx{ VK_INVALID_INDEX };
	for (uint32 i = 0; i < queueFamilyCount; ++i) {
		const VkQueueFamilyProperties& prop = queueFamilyProperties[i];
		if ((prop.queueFlags & VK_QUEUE_GRAPHICS_BIT) == VK_QUEUE_GRAPHICS_BIT && VK_INVALID_INDEX == graphicsQueueFamilyIdx) {
			graphicsQueueFamilyIdx = i;
		}
		if ((prop.queueFlags & VK_QUEUE_COMPUTE_BIT) == VK_QUEUE_COMPUTE_BIT && VK_INVALID_INDEX == computeQueueFamilyIdx) {
			computeQueueFamilyIdx = i;
		}
		if ((prop.queueFlags & VK_QUEUE_TRANSFER_BIT) == VK_QUEUE_TRANSFER_BIT && VK_INVALID_INDEX == transferQueueFamilyIdx) {
			transferQueueFamilyIdx = i;
		}
	}

	TSet<uint32> queueFamilyIndices{ graphicsQueueFamilyIdx, computeQueueFamilyIdx, transferQueueFamilyIdx };
	uint32 queueCount = (uint32)queueFamilyIndices.size();
	TFixedArray<VkDeviceQueueCreateInfo> queueCreateInfos(queueCount);
	static const float priority = 1.0f;
	constexpr uint32 queueCreateCount = 1u;// Always create ONE queue.
	queueCount = 0;
	for (uint32 queueFamilyIndex : queueFamilyIndices) {
		VkDeviceQueueCreateInfo& info = queueCreateInfos[queueCount++];
		info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		info.pNext = nullptr;
		info.flags = 0;
		info.queueFamilyIndex = queueFamilyIndex;
		info.queueCount = queueCreateCount;
		info.pQueuePriorities = &priority;
	}

	const uint32 apiVersion = context->GetAPIVersion();
	ASSERT(apiVersion >= VK_VERSION_1_2, "");

	// fill device extensions
	TArray<const char*> extensions = GetDeviceExtensions(context);
	// setup features
	VkPhysicalDeviceFeatures2 features2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
	VkPhysicalDeviceVulkan11Features features11{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
	VkPhysicalDeviceVulkan12Features features12{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	VkPhysicalDeviceVulkan13Features features13{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	features2.pNext = &features11;
	features11.pNext = &features12;
	VkPhysicalDeviceFeatures& features = features2.features;
	features.samplerAnisotropy = VK_TRUE;
	features.fragmentStoresAndAtomics = VK_TRUE;
	features.independentBlend = VK_TRUE;

	// create device
	VkDeviceCreateInfo deviceCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
	deviceCreateInfo.flags = 0;
	//  dynamic rendering must be supported.
	if (apiVersion < VK_VERSION_1_3) {
		deviceCreateInfo.pEnabledFeatures = &features;
		deviceCreateInfo.pNext = nullptr;
	}
	else {
		features13.dynamicRendering = true;
		features12.pNext = &features13;
		deviceCreateInfo.pEnabledFeatures = nullptr;
		deviceCreateInfo.pNext = &features2;
	}
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.Data();
	deviceCreateInfo.queueCreateInfoCount = queueCount;
	deviceCreateInfo.enabledExtensionCount = extensions.Size();
	deviceCreateInfo.ppEnabledExtensionNames = extensions.Data();
	deviceCreateInfo.enabledLayerCount = 0;
	VK_ASSERT(vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_Device), "vkCreateDevice");
	LOG_DEBUG("[VulkanDevice::CreateDevice] Initialized GPU: %s, %u", m_DeviceProperties.deviceName, m_DeviceProperties.deviceID);

	// load device functions
	LoadDeviceFunctions(m_Device);
	// compatible with vulkan 1.2
	if(apiVersion < VK_VERSION_1_3) {
		vkCmdBeginRendering = reinterpret_cast<PFN_vkCmdBeginRenderingKHR>(vkGetDeviceProcAddr(m_Device, "vkCmdBeginRenderingKHR"));
		vkCmdEndRendering = reinterpret_cast<PFN_vkCmdEndRenderingKHR>(vkGetDeviceProcAddr(m_Device, "vkCmdEndRenderingKHR"));
	}

	// get queues
	constexpr uint32 fixedQueueIndex = 0;// Always get the first queue.
	VulkanQueue& graphicsQueue = m_Queues[EnumCast(EQueueType::Graphics)];
	graphicsQueue.FamilyIndex = graphicsQueueFamilyIdx;
	graphicsQueue.QueueIndex = fixedQueueIndex;
	vkGetDeviceQueue(m_Device, graphicsQueueFamilyIdx, fixedQueueIndex, &graphicsQueue.Handle);
	VulkanQueue& computeQueue = m_Queues[EnumCast(EQueueType::Compute)];
	computeQueue.FamilyIndex = computeQueueFamilyIdx;
	computeQueue.QueueIndex = fixedQueueIndex;
	vkGetDeviceQueue(m_Device, computeQueueFamilyIdx, fixedQueueIndex, &computeQueue.Handle);
	VulkanQueue& transferQueue = m_Queues[EnumCast(EQueueType::Transfer)];
	transferQueue.FamilyIndex = transferQueueFamilyIdx;
	transferQueue.QueueIndex = fixedQueueIndex;
	vkGetDeviceQueue(m_Device, transferQueueFamilyIdx, fixedQueueIndex, &transferQueue.Handle);
}
