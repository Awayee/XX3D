#include "VulkanUtil.h"
#include "VulkanFuncs.h"
#include "Resource/Public/Config.h"

namespace Engine {

	void FindQueueFamilyIndex(const VkPhysicalDevice& device, const VkSurfaceKHR& surface, int* pGraphicsIndex, int* pPresentIndex, int* pComputeIndex)
	{
		*pGraphicsIndex = -1;
		*pPresentIndex = -1;
		*pComputeIndex = -1;
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

			if (*pGraphicsIndex != -1 && *pPresentIndex != -1 && *pComputeIndex != -1) break;
		}
	}

	bool CheckExtensionSupported(VkPhysicalDevice device, const TVector<const char*>& extensions) {
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
	VkFormat FindDepthFormat(VkPhysicalDevice physicalDevice) {
		// find depth format
		const TVector<VkFormat> candidates{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
		VkImageTiling tiling{ VK_IMAGE_TILING_OPTIMAL };
		VkFormatFeatureFlags features{ VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT };
		for (VkFormat format : candidates)
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
			{
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
			{
				return format;
			}
		}
		ERROR("findSupportedFormat failed");
		return VK_FORMAT_UNDEFINED;
	}

	bool CheckValidationLayerSupport(const TVector<const char*>& validationLayers)
	{
		uint32 layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		TVector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.Data());
		for (auto& name: validationLayers)
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

	SPhyicalDeviceInfo GetPhysicalDeviceInfo(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, const TVector<const char*>& extensions){
		SPhyicalDeviceInfo info;
		if (!CheckExtensionSupported(physicalDevice, extensions)) {
			return info;
		}
		// query family index
		FindQueueFamilyIndex(physicalDevice, surface, &info.graphicsIndex, &info.presentIndex, &info.computeIndex);
		if (-1 == info.graphicsIndex || -1 == info.presentIndex || -1 == info.computeIndex) {
			return info;
		}

		// query swapchain info
		// format
		uint32 formatCount;
		VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr), "vkGetPhysicalDeviceSurfaceFormatsKHR");
		TVector<VkSurfaceFormatKHR> formats(formatCount);
		VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.Data()), "vkGetPhysicalDeviceSurfaceFormatsKHR");
		for (const auto& format : formats) {
			if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				info.swapchainFormat = format;
				break;
			}
		}
		//present mode
		uint32 presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
		TVector<VkPresentModeKHR> presentModes(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.Data());
		if(presentModes.Empty()) {
			return info;
		}
		presentModes.Sort([](const VkPresentModeKHR& v1, const VkPresentModeKHR& v2) {
			VkPresentModeKHR sortList[4]{
				VK_PRESENT_MODE_IMMEDIATE_KHR,
				VK_PRESENT_MODE_MAILBOX_KHR,
				VK_PRESENT_MODE_FIFO_KHR,
				VK_PRESENT_MODE_FIFO_RELAXED_KHR
			};
			int idx1{ INT16_MAX }, idx2{ INT16_MAX };
			for (int i = 0; i < std::size(sortList); ++i) {
				if (i == v1) idx1 = i;
				if (i == v2) idx2 = i;
			}
			return idx1 < idx2;
		});
		info.swapchainPresentMode = presentModes[0];

		//extent
		VkSurfaceCapabilitiesKHR capabilities;
		VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities), "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
		if (capabilities.currentExtent.width != std::numeric_limits<uint32>::max()) {
			info.swapchainExtent = capabilities.currentExtent;
		}
		info.imageCount = capabilities.minImageCount + 1;
		if (capabilities.maxImageCount > 0 && info.imageCount > capabilities.maxImageCount) {
			info.imageCount = capabilities.maxImageCount;
		}
		info.swapchainTransform = capabilities.currentTransform;

		if (info.swapchainFormat.format == VK_FORMAT_UNDEFINED || info.swapchainPresentMode == VK_PRESENT_MODE_MAX_ENUM_KHR || info.swapchainExtent.width == 0) {
			return info;
		}

		info.score = 1;

		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);

		// discrete gpu is first
		auto gpuType = GetConfig().GPUType;
		ASSERT(gpuType != GPU_NONE, "Invalid GPU!");
		VkPhysicalDeviceType preferredType = gpuType == GPU_DISCRETE ? VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU : VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
		if(properties.deviceType == preferredType) {
			info.score += 100;
		}
		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceFeatures(physicalDevice, &features);
		if (features.geometryShader) {
			info.score += 10;
		}

		return info;
	}


	void GetPipelineBarrierStage(VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags& srcAccessMask, VkAccessFlags& dstAccessMask, VkPipelineStageFlags& srcStage, VkPipelineStageFlags& dstStage) {
		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			srcAccessMask = 0;
			dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
			newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		// for getGuidAndDepthOfMouseClickOnRenderSceneForUI() get depthimage
		else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL &&
			newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			srcStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL &&
			newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		// for generating mipmapped image
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
			newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else
		{
			ERROR("unsupported layout transition!");
		}
	}

	void GenerateMipMap(VkCommandBuffer cmd, VkImage image, uint32 levelCount, uint32 width, uint32 height, VkImageAspectFlags aspect, uint32 baseLayer, uint32 layerCount) {
		for (uint32 i = 1; i < levelCount; i++)
		{
			VkImageBlit imageBlit{};
			imageBlit.srcSubresource.aspectMask = aspect;
			imageBlit.srcSubresource.baseArrayLayer = baseLayer;
			imageBlit.srcSubresource.layerCount = layerCount;
			imageBlit.srcSubresource.mipLevel = i - 1;
			imageBlit.srcOffsets[1].x = std::max((int32_t)(width >> (i - 1)), 1);
			imageBlit.srcOffsets[1].y = std::max((int32_t)(height >> (i - 1)), 1);
			imageBlit.srcOffsets[1].z = 1;

			imageBlit.dstSubresource.aspectMask = aspect;
			imageBlit.dstSubresource.layerCount = layerCount;
			imageBlit.dstSubresource.mipLevel = i;
			imageBlit.dstOffsets[1].x = std::max((int32_t)(width >> i), 1);
			imageBlit.dstOffsets[1].y = std::max((int32_t)(height >> i), 1);
			imageBlit.dstOffsets[1].z = 1;

			VkImageSubresourceRange mipSubRange{};
			mipSubRange.aspectMask = aspect;
			mipSubRange.baseMipLevel = i;
			mipSubRange.levelCount = 1;
			mipSubRange.layerCount = layerCount;

			VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = image;
			barrier.subresourceRange = mipSubRange;

			vkCmdPipelineBarrier(cmd,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0,
				nullptr,
				0,
				nullptr,
				1,
				&barrier);

			vkCmdBlitImage(cmd,
				image,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&imageBlit,
				VK_FILTER_LINEAR);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(cmd,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0,
				nullptr,
				0,
				nullptr,
				1,
				&barrier);
		}

		VkImageSubresourceRange mipSubRange{};
		mipSubRange.aspectMask = aspect;
		mipSubRange.baseMipLevel = 0;
		mipSubRange.levelCount = levelCount;
		mipSubRange.baseArrayLayer = baseLayer;
		mipSubRange.layerCount = layerCount;

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange = mipSubRange;

		vkCmdPipelineBarrier(cmd,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&barrier);
	}
}