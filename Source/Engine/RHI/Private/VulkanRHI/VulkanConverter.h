#pragma once
#include "VulkanCommon.h"
#include "RHI/Public/RHIEnum.h"
#include "RHI/Public/RHIResources.h"

VkFormat ToVkFormat(ERHIFormat f);
VkBufferUsageFlags ToBufferUsage(EBufferFlags flags);
VkMemoryPropertyFlags ToBufferMemoryProperty(EBufferFlags flags);
VkMemoryPropertyFlags ToImageMemoryProperty(ETextureFlags flags);
VkImageType ToImageType(ETextureDimension dimension);
VkImageCreateFlags ToImageCreateFlags(ETextureDimension dimension);
VkImageViewType ToImageViewType(ETextureDimension dimension);
VkImageUsageFlags ToImageUsage(ETextureFlags flags);
VkImageAspectFlags ToImageAspectFlags(ETextureFlags flags);
VkFilter ToFilter(ESamplerFilter filter);
VkSamplerAddressMode ToAddressMode(ESamplerAddressMode mode);
VkShaderStageFlagBits ToVkShaderStageFlagBit(EShaderStageFlagBit type);
VkShaderStageFlags ToVkShaderStageFlags(EShaderStageFlags flags);
VkDescriptorType ToVkDescriptorType(EBindingType type);
VkPrimitiveTopology ToPrimitiveTopology(EPrimitiveTopology topology);
VkPolygonMode ToPolygonMode(ERasterizerFill fill);
VkCullModeFlagBits ToVkCullMode(ERasterizerCull cull);
VkSampleCountFlagBits ToVkMultiSampleCount(uint8 count);
VkCompareOp ToVkCompareOp(ECompareType ConvertCompareOptype);
VkStencilOpState ToVkStencilOpState(const RHIDepthStencilState::StencilOpDesc& desc);
VkPipelineRasterizationStateCreateInfo ToRasterizationStateCreateInfo(const RHIRasterizerState& desc);
VkPipelineDepthStencilStateCreateInfo ToDepthStencilStateCreateInfo(const RHIDepthStencilState& desc);
VkBlendOp ToVkBlendOp(EBlendOption op);
VkPipelineColorBlendAttachmentState ToAttachmentBlendState(const RHIBlendState& blendState);
VkAttachmentLoadOp ToVkAttachmentLoadOp(ERTLoadOp op);
VkAttachmentStoreOp ToVkAttachmentStoreOp(ERTStoreOp op);
VkImageLayout ToImageLayout(EResourceState state);
