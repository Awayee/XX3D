#pragma once
#include "VulkanCommon.h"
#include "RHI/Public/RHIEnum.h"
#include "RHI/Public/RHIResources.h"

inline VkFormat ToVkFormat(ERHIFormat f);
inline VkBufferUsageFlags ToBufferUsage(EBufferFlags flags);
inline VkMemoryPropertyFlags ToBufferMemoryProperty(EBufferFlags flags);
inline VkMemoryPropertyFlags ToImageMemoryProperty(ETextureFlags flags);
inline VkImageType ToImageType(ETextureDimension dimension);
inline VkImageViewType ToImageViewType(ETextureDimension dimension);
inline VkImageUsageFlags ToImageUsage(ETextureFlags flags);
inline VkImageAspectFlags ToImageAspectFlags(ETextureFlags flags);
inline VkFilter ToFilter(ESamplerFilter filter);
inline VkSamplerAddressMode ToAddressMode(ESamplerAddressMode mode);
inline VkShaderStageFlagBits ToVkShaderStageFlagBit(EShaderStageFlagBit type);
inline VkShaderStageFlags ToVkShaderStageFlags(EShaderStageFlags flags);
inline VkDescriptorType ToVkDescriptorType(EBindingType type);
inline VkPrimitiveTopology ToPrimitiveTopology(EPrimitiveTopology topology);
inline VkPolygonMode ToPolygonMode(ERasterizerFill fill);
inline VkCullModeFlagBits ToVkCullMode(ERasterizerCull cull);
inline VkSampleCountFlagBits ToVkMultiSampleCount(uint8 count);
inline VkCompareOp ToVkCompareOp(ECompareType ConvertCompareOptype);
inline VkStencilOpState ToVkStencilOpState(const RHIDepthStencilState::StencilOpDesc& desc);
inline VkPipelineRasterizationStateCreateInfo ToRasterizationStateCreateInfo(const RHIRasterizerState& desc);
inline VkPipelineDepthStencilStateCreateInfo ToDepthStencilStateCreateInfo(const RHIDepthStencilState& desc);
inline VkBlendOp ToVkBlendOp(EBlendOption op);
inline VkPipelineColorBlendAttachmentState ToAttachmentBlendState(const RHIBlendState& blendState);
inline VkAttachmentLoadOp ToVkAttachmentLoadOp(ERTLoadOp op);
inline VkAttachmentStoreOp ToVkAttachmentStoreOp(ERTStoreOp op);
