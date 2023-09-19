#pragma once
#include "Core/Public/macro.h"
#include "RHIVKResources.h"
#include "RHIVKClasses.h"
#include "VulkanConverter.h"

#define VK_CHECK(x, s)\
	if(VK_SUCCESS != x) throw s

	const uint32 VK_API_VER = VK_API_VERSION_1_2;

#define VK_FREE(ptr) delete ptr; ptr = nullptr

using namespace Engine;

inline VkPipelineRasterizationStateCreateInfo TranslateVkPipelineRasterizationState(const RGraphicsPipelineCreateInfo& rhiInfo) {
	VkPipelineRasterizationStateCreateInfo rasterizatonInfo{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, nullptr, 0 };
	rasterizatonInfo.rasterizerDiscardEnable = rhiInfo.RasterizerDiscardEnable;
	rasterizatonInfo.polygonMode = (VkPolygonMode)rhiInfo.PolygonMode;
	rasterizatonInfo.cullMode = rhiInfo.CullMode;
	rasterizatonInfo.frontFace = rhiInfo.Clockwise ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizatonInfo.depthBiasEnable = rhiInfo.DepthBiasEnable;
	rasterizatonInfo.depthBiasConstantFactor = rhiInfo.DepthBiasConstantFactor;
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
	desc.format =                ToVkFormat(attachment.Format);
	desc.samples =        (VkSampleCountFlagBits)attachment.SampleCount;
	desc.loadOp =            (VkAttachmentLoadOp)attachment.LoadOp;
	desc.storeOp =          (VkAttachmentStoreOp)attachment.StoreOp;
	desc.stencilLoadOp =     (VkAttachmentLoadOp)attachment.StencilLoadOp;
	desc.stencilStoreOp =   (VkAttachmentStoreOp)attachment.StencilStoreOp;
	desc.initialLayout =    ToVkImageLayout(attachment.InitialLayout);
	desc.finalLayout =      ToVkImageLayout(attachment.FinalLayout);
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

void GetPipelineBarrierStage(VkImageLayout oldLayout, VkImageLayout newLayout,
	VkAccessFlags& srcAccessMask, VkAccessFlags& dstAccessMask, VkPipelineStageFlags& srcStage, VkPipelineStageFlags& dstStage);

void GenerateMipMap(VkCommandBuffer cmd, VkImage image, uint32 levelCount, uint32 width, uint32 height,
	VkImageAspectFlags aspect, uint32 baseLayer, uint32 layerCount);