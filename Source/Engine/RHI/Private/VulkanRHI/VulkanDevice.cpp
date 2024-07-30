#include "VulkanDevice.h"
#include "Core/Public/TArray.h"
#include "Core/Public/Container.h"
#include "VulkanMemory.h"
#include "VulkanDescriptorSet.h"
#include "VulkanCommand.h"
#include "VulkanUploader.h"

inline TVector<XXString> GetDeviceExtensions(const VulkanContext* context) {
	TVector<XXString> extensions;
	extensions.PushBack(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	const uint32 APIVersion = context->GetAPIVersion();
	if(APIVersion < VK_API_VERSION_1_2) {
		extensions.PushBack(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
	}
	return extensions;
}

VulkanDevice::VulkanDevice(const VulkanContext* context, VkPhysicalDevice physicalDevice): m_PhysicalDevice(physicalDevice) {
	// Get device properties
	vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_DeviceProperties);
	// Create logical device
	CreateDevice(context);
	// Create memory manager
	m_MemoryMgr.Reset(new VulkanMemoryMgr(context, this));
	// Create descriptor manager
	m_DescriptorMgr.Reset(new VulkanDescriptorMgr(this));
	// Create command manager
	m_CommandMgr.Reset(new VulkanCommandMgr(this));
	// Create uploader
	m_Uploader.Reset(new VulkanUploader(this));
}

VulkanDevice::~VulkanDevice() {
}

const VulkanQueue* VulkanDevice::FindPresentQueue(VkSurfaceKHR surface) const {
	VkBool32 isSupport = VK_FALSE;
	vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, m_GraphicsQueue.FamilyIndex, surface, &isSupport);
	if(isSupport) {
		return &m_GraphicsQueue;
	}
	vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, m_ComputeQueue.FamilyIndex, surface, &isSupport); 
	if(isSupport) {
		return &m_ComputeQueue;
	}
	return nullptr;
}

void VulkanDevice::CreateDevice(const VulkanContext* context) {
	// Get queue
	uint32 queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);
	TFixedArray<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, queueFamilyProperties.Data());

	// get queue indices
	// the present queue will be created after swapchain
	uint32 graphicsQueueFamilyIdx{ VK_INVALID_IDX }, computeQueueFamilyIdx{ VK_INVALID_IDX }, transferQueueFamilyIdx{ VK_INVALID_IDX };
	for (uint32 i = 0; i < queueFamilyCount; ++i) {
		const VkQueueFamilyProperties& prop = queueFamilyProperties[i];
		if ((prop.queueFlags & VK_QUEUE_GRAPHICS_BIT) == VK_QUEUE_GRAPHICS_BIT && VK_INVALID_IDX == graphicsQueueFamilyIdx) {
			graphicsQueueFamilyIdx = i;
		}
		if ((prop.queueFlags & VK_QUEUE_COMPUTE_BIT) == VK_QUEUE_COMPUTE_BIT && VK_INVALID_IDX == computeQueueFamilyIdx) {
			computeQueueFamilyIdx = i;
		}
		if ((prop.queueFlags & VK_QUEUE_TRANSFER_BIT) == VK_QUEUE_TRANSFER_BIT && VK_INVALID_IDX == transferQueueFamilyIdx) {
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
	TVector<const char*> extensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
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
	m_GraphicsQueue.FamilyIndex = graphicsQueueFamilyIdx;
	m_GraphicsQueue.QueueIndex = fixedQueueIndex;
	vkGetDeviceQueue(m_Device, graphicsQueueFamilyIdx, fixedQueueIndex, &m_GraphicsQueue.Handle);
	m_ComputeQueue.FamilyIndex = computeQueueFamilyIdx;
	m_ComputeQueue.QueueIndex = fixedQueueIndex;
	vkGetDeviceQueue(m_Device, computeQueueFamilyIdx, fixedQueueIndex, &m_ComputeQueue.Handle);
	m_TransferQueue.FamilyIndex = transferQueueFamilyIdx;
	m_TransferQueue.QueueIndex = fixedQueueIndex;
	vkGetDeviceQueue(m_Device, transferQueueFamilyIdx, fixedQueueIndex, &m_TransferQueue.Handle);
}
