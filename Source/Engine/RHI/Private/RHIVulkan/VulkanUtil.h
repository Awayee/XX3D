#pragma once
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include "Core/Public/macro.h"
#include "Core/Public/TVector.h"
#include "RHIVKResources.h"
#include "VulkanEnum.h"

namespace Engine {

#define VK_CHECK(x, s)\
	if(VK_SUCCESS != x) throw s

	const uint32 VK_API_VER = VK_API_VERSION_1_2;

#define VK_FREE(ptr) delete ptr; ptr = nullptr

	// debug callback
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT,
		VkDebugUtilsMessageTypeFlagsEXT,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* data)
	{
		ERROR("[Vulkan Error] %s", pCallbackData->pMessage);
		return VK_FALSE;
	}

	inline void SetDebugInfo(VkDebugUtilsMessengerCreateInfoEXT& debugInfo) {
		debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugInfo.pfnUserCallback = DebugCallback;
	}


	inline VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger)
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) {
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		}
		else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	inline void DestroyDebugUtilsMessengerEXT(VkInstance instance,
		VkDebugUtilsMessengerEXT     debugMessenger,
		const VkAllocationCallbacks* pAllocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr)
		{
			func(instance, debugMessenger, pAllocator);
		}
	}

	inline VkPipelineRasterizationStateCreateInfo TranslateVkPipelineRasterizationState(const RGraphicsPipelineCreateInfo& rhiInfo) {
		VkPipelineRasterizationStateCreateInfo rasterizatonInfo{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, nullptr, 0 };
		rasterizatonInfo.depthBiasClamp = rhiInfo.DepthBiasClamp;
		rasterizatonInfo.rasterizerDiscardEnable = rhiInfo.RasterizerDiscardEnable;
		rasterizatonInfo.polygonMode = (VkPolygonMode)rhiInfo.PolygonMode;
		rasterizatonInfo.cullMode = rhiInfo.CullMode;
		rasterizatonInfo.frontFace = rhiInfo.Clockwise ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizatonInfo.depthBiasEnable = rhiInfo.DepthBiasEnable;
		rasterizatonInfo.depthBiasSlopeFactor = rhiInfo.DepthBiasConstantFactor;
		rasterizatonInfo.depthBiasClamp = rhiInfo.DepthBiasClamp;
		rasterizatonInfo.depthBiasSlopeFactor = rhiInfo.DepthBiasSlopeFactor;
		rasterizatonInfo.lineWidth = rhiInfo.LineWidth;
		return rasterizatonInfo;
	}

	inline void TranslateVkStencilOp(VkStencilOpState& vkState, const RStencilOpState& rhiState) {
		vkState.failOp = (VkStencilOp)rhiState.failOp;
		vkState.passOp = (VkStencilOp)rhiState.passOp;
		vkState.depthFailOp = (VkStencilOp)rhiState.depthFailOp;
		vkState.compareOp = (VkCompareOp)rhiState.compareOp;
		vkState.compareMask = rhiState.compareMask;
		vkState.writeMask = rhiState.writeMask;
		vkState.reference = rhiState.reference;
	}

	inline void TranslateColorBlendAttachmentState(VkPipelineColorBlendAttachmentState& vkState, const RColorBlendAttachmentState& rhiState) {
		vkState.blendEnable = rhiState.blendEnable;
		vkState.srcColorBlendFactor = (VkBlendFactor)rhiState.srcColorBlendFactor;
		vkState.dstColorBlendFactor = (VkBlendFactor)rhiState.dstColorBlendFactor;
		vkState.colorBlendOp = (VkBlendOp)rhiState.colorBlendOp;
		vkState.srcAlphaBlendFactor = (VkBlendFactor)rhiState.srcAlphaBlendFactor;
		vkState.dstAlphaBlendFactor = (VkBlendFactor)rhiState.dstAlphaBlendFactor;
		vkState.alphaBlendOp = (VkBlendOp)rhiState.alphaBlendOp;
		vkState.colorWriteMask = (VkColorComponentFlags)rhiState.colorWriteMask;
	}

	inline VkPipelineMultisampleStateCreateInfo TranslatePipelineMultisample(const RGraphicsPipelineCreateInfo& info) {
		VkPipelineMultisampleStateCreateInfo multisampleInfo{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, nullptr, 0 };
		multisampleInfo.rasterizationSamples = (VkSampleCountFlagBits)info.RasterizationSamples;
		multisampleInfo.sampleShadingEnable = info.SampleShadingEnable;
		multisampleInfo.minSampleShading = info.MinSampleShading;
		multisampleInfo.pSampleMask = info.pSampleMask;
		multisampleInfo.alphaToCoverageEnable = info.AlphaToCoverageEnable;
		multisampleInfo.alphaToOneEnable = info.AlphaToOneEnable;
		return multisampleInfo;
	}

	inline VkPipelineDepthStencilStateCreateInfo TranslatePipelineDepthStencil(const RGraphicsPipelineCreateInfo& info) {
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, nullptr, 0 };
		depthStencilInfo.depthTestEnable = info.DepthTestEnable;
		depthStencilInfo.depthWriteEnable = info.DepthWriteEnable;
		depthStencilInfo.depthCompareOp = (VkCompareOp)info.DepthCompareOp;
		depthStencilInfo.depthBoundsTestEnable = info.DepthBoundsTestEnable;
		depthStencilInfo.stencilTestEnable = info.StencilTestEnable;
		TranslateVkStencilOp(depthStencilInfo.front, info.FrontStencilOp);
		TranslateVkStencilOp(depthStencilInfo.back, info.BackStencilOp);
		depthStencilInfo.minDepthBounds = info.MinDepthBounds;
		depthStencilInfo.maxDepthBounds = info.MaxDepthBounds;
		return depthStencilInfo;
	}

	inline VkAttachmentDescription ResolveAttachmentDesc(const RSAttachment& attachment) {
		VkAttachmentDescription desc;
		desc.flags = 0;
		desc.format =                ConvertVkFormat(attachment.Format);
		desc.samples =        (VkSampleCountFlagBits)attachment.SampleCount;
		desc.loadOp =            (VkAttachmentLoadOp)attachment.LoadOp;
		desc.storeOp =          (VkAttachmentStoreOp)attachment.StoreOp;
		desc.stencilLoadOp =     (VkAttachmentLoadOp)attachment.StencilLoadOp;
		desc.stencilStoreOp =   (VkAttachmentStoreOp)attachment.StencilStoreOp;
		desc.initialLayout =    ConvertVkImageLayout(attachment.InitialLayout);
		desc.finalLayout =      ConvertVkImageLayout(attachment.FinalLayout);
		return desc;
	}

	inline VkSubpassDependency ResolveSubpassDependency(const RSubpassDependency& dependency) {
		VkSubpassDependency d{};
		d.srcSubpass = dependency.SrcSubPass;
		d.srcStageMask = dependency.SrcStage;
		d.srcAccessMask = dependency.SrcAccess;
		d.dstSubpass = dependency.DstSubPass;
		d.dstStageMask = dependency.DstStage;
		d.dstAccessMask = dependency.DstAccess;
		return d;
	}

	inline VkClearValue ResolveClearValue(const RSClear& clear) {
		VkClearValue clearVk;
		if(CLEAR_VALUE_COLOR == clear.Type) {
			clearVk.color.float32[0] = clear.Color.r;
			clearVk.color.float32[1] = clear.Color.g;
			clearVk.color.float32[2] = clear.Color.b;
			clearVk.color.float32[3] = clear.Color.a;
		}
		else if(CLEAR_VALUE_DEPTH_STENCIL == clear.Type) {
			clearVk.depthStencil.depth = clear.DepthStencil.depth;
			clearVk.depthStencil.stencil = clear.DepthStencil.stencil;
		}
		return clearVk;
	}

	void FindQueueFamilyIndex(const VkPhysicalDevice& device, const VkSurfaceKHR& surface, int* pGraphicsIndex, int* pPresentIndex, int* pComputeIndex);

	bool CheckExtensionSupported(VkPhysicalDevice device, const TVector<const char*>& extensions);

	VkFormat FindDepthFormat(VkPhysicalDevice physicalDevice);

	bool CheckValidationLayerSupport(const TVector<const char*>& validationLayers);

	struct SPhyicalDeviceInfo {
		int score{ -1 };
		int graphicsIndex{ -1 };
		int presentIndex{ -1 };
		int computeIndex{ -1 };
		uint32 imageCount{ 0 };
		VkSurfaceFormatKHR swapchainFormat{ VK_FORMAT_UNDEFINED };
		VkPresentModeKHR swapchainPresentMode{ VK_PRESENT_MODE_MAX_ENUM_KHR };
		VkSurfaceTransformFlagBitsKHR swapchainTransform{ VK_SURFACE_TRANSFORM_FLAG_BITS_MAX_ENUM_KHR };
		VkExtent2D swapchainExtent{ 0, 0 };
	};

	SPhyicalDeviceInfo GetPhysicalDeviceInfo(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, const TVector<const char*>& extensions);


	void GetPipelineBarrierStage(VkImageLayout oldLayout, VkImageLayout newLayout,
		VkAccessFlags& srcAccessMask, VkAccessFlags& dstAccessMask, VkPipelineStageFlags& srcStage, VkPipelineStageFlags& dstStage);

	void GenerateMipMap(VkCommandBuffer cmd, VkImage image, uint32 levelCount, uint32 width, uint32 height,
		VkImageAspectFlags aspect, uint32 baseLayer, uint32 layerCount);

}