#include "VulkanConverter.h"
#include "VulkanResources.h"

static VkFormat s_FormatMap[(uint32)ERHIFormat::FORMAT_MAX_ENUM] = {
	VK_FORMAT_UNDEFINED,
	VK_FORMAT_R8_UNORM,
	VK_FORMAT_R8_SNORM,
	VK_FORMAT_R8_UINT,
	VK_FORMAT_R8_SINT,
	VK_FORMAT_R8G8_UNORM,
	VK_FORMAT_R8G8_SNORM,
	VK_FORMAT_R8G8_UINT,
	VK_FORMAT_R8G8_SINT,
	VK_FORMAT_R8G8B8A8_UNORM,
	VK_FORMAT_R8G8B8A8_SNORM,
	VK_FORMAT_R8G8B8A8_UINT,
	VK_FORMAT_R8G8B8A8_SINT,
	VK_FORMAT_R8G8B8A8_SRGB,
	VK_FORMAT_B8G8R8A8_UNORM,
	VK_FORMAT_B8G8R8A8_SRGB,
	VK_FORMAT_R16_UNORM,
	VK_FORMAT_R16_SNORM,
	VK_FORMAT_R16_UINT,
	VK_FORMAT_R16_SINT,
	VK_FORMAT_R16_SFLOAT,
	VK_FORMAT_R16G16_UNORM,
	VK_FORMAT_R16G16_SNORM,
	VK_FORMAT_R16G16_UINT,
	VK_FORMAT_R16G16_SINT,
	VK_FORMAT_R16G16_SFLOAT,
	VK_FORMAT_R16G16B16A16_UNORM,
	VK_FORMAT_R16G16B16A16_SNORM,
	VK_FORMAT_R16G16B16A16_UINT,
	VK_FORMAT_R16G16B16A16_SINT,
	VK_FORMAT_R16G16B16A16_SFLOAT,
	VK_FORMAT_R32_UINT,
	VK_FORMAT_R32_SINT,
	VK_FORMAT_R32_SFLOAT,
	VK_FORMAT_R32G32_UINT,
	VK_FORMAT_R32G32_SINT,
	VK_FORMAT_R32G32_SFLOAT,
	VK_FORMAT_R32G32B32_UINT,
	VK_FORMAT_R32G32B32_SINT,
	VK_FORMAT_R32G32B32_SFLOAT,
	VK_FORMAT_R32G32B32A32_UINT,
	VK_FORMAT_R32G32B32A32_SINT,
	VK_FORMAT_R32G32B32A32_SFLOAT,
	VK_FORMAT_D16_UNORM,
	VK_FORMAT_D24_UNORM_S8_UINT,
	VK_FORMAT_D32_SFLOAT
};

VkFormat ToVkFormat(ERHIFormat f) {
	return s_FormatMap[(uint32)f];
}

 VkBufferUsageFlags ToBufferUsage(EBufferFlags flags) {
	VkBufferUsageFlags usage {0};
	if (flags & EBufferFlagBit::BUFFER_FLAG_COPY_SRC) {
		usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	}
	if (flags & EBufferFlagBit::BUFFER_FLAG_COPY_DST) {
		usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}
	if (flags & EBufferFlagBit::BUFFER_FLAG_UNIFORM) {
		usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	}
	if (flags & EBufferFlagBit::BUFFER_FLAG_INDEX) {
		usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	}
	if (flags & EBufferFlagBit::BUFFER_FLAG_VERTEX) {
		usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	}
	if (flags & EBufferFlagBit::BUFFER_FLAG_STORAGE) {
		usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	}
	if (flags & EBufferFlagBit::BUFFER_FLAG_INDIRECT) {
		usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
	}
	return usage;
}

 VkMemoryPropertyFlags ToBufferMemoryProperty(EBufferFlags flags) {
	if (flags & (EBufferFlagBit::BUFFER_FLAG_COPY_SRC | EBufferFlagBit::BUFFER_FLAG_UNIFORM | EBufferFlagBit::BUFFER_FLAG_STORAGE)) {
		return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	}
	if (flags & (EBufferFlagBit::BUFFER_FLAG_INDEX | EBufferFlagBit::BUFFER_FLAG_VERTEX)) {
		return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	}
	return 0;
}

 VkMemoryPropertyFlags ToImageMemoryProperty(ETextureFlags flags) {
	if (flags & (ETextureFlagBit::TEXTURE_FLAG_CPY_SRC)) {
		return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	}
	return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
}

 VkImageType ToImageType(ETextureDimension dimension) {
	switch (dimension) {
	case ETextureDimension::Tex2D:
	case ETextureDimension::Tex2DArray:
	case ETextureDimension::TexCube:
	case ETextureDimension::TexCubeArray: return VK_IMAGE_TYPE_2D;
	case ETextureDimension::Tex3D: return VK_IMAGE_TYPE_3D;
	}
	return VK_IMAGE_TYPE_MAX_ENUM;
}

VkImageCreateFlags ToImageCreateFlags(ETextureDimension dimension) {
	switch (dimension) {
	case ETextureDimension::Tex2D: 
	case ETextureDimension::Tex2DArray: return 0;
	case ETextureDimension::TexCube:
	case ETextureDimension::TexCubeArray: return VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	case ETextureDimension::Tex3D: return 0;
	}
	return 0;
}

VkImageViewType ToImageViewType(ETextureDimension dimension) {
	switch (dimension) {
	case ETextureDimension::Tex2D: return VK_IMAGE_VIEW_TYPE_2D;
	case ETextureDimension::Tex2DArray: return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	case ETextureDimension::TexCube: return VK_IMAGE_VIEW_TYPE_CUBE;
	case ETextureDimension::TexCubeArray: return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
	case ETextureDimension::Tex3D: return VK_IMAGE_VIEW_TYPE_3D;
	}
	return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
}

 VkImageUsageFlags ToImageUsage(ETextureFlags flags) {
	VkImageUsageFlags vkFlags = 0;
	if (flags & (ETextureFlagBit::TEXTURE_FLAG_DEPTH_TARGET | ETextureFlagBit::TEXTURE_FLAG_STENCIL)) {
		vkFlags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	}
	if (flags & ETextureFlagBit::TEXTURE_FLAG_COLOR_TARGET) {
		vkFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}
	if (flags & ETextureFlagBit::TEXTURE_FLAG_SRV) {
		vkFlags |= VK_IMAGE_USAGE_SAMPLED_BIT;
	}
	if (flags & ETextureFlagBit::TEXTURE_FLAG_CPY_DST) {
		vkFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}
	if (flags & ETextureFlagBit::TEXTURE_FLAG_CPY_SRC) {
		vkFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}
	if (flags & ETextureFlagBit::TEXTURE_FLAG_UAV) {
		vkFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
	}
	if (flags & ETextureFlagBit::TEXTURE_FLAG_PRESENT) {
	}
	return vkFlags;
}

 VkImageAspectFlags ToImageAspectFlags(ETextureFlags flags) {
	VkImageAspectFlags vkFlags = 0;
	if (flags & (ETextureFlagBit::TEXTURE_FLAG_SRV | ETextureFlagBit::TEXTURE_FLAG_COLOR_TARGET | ETextureFlagBit::TEXTURE_FLAG_PRESENT)) {
		vkFlags |= VK_IMAGE_ASPECT_COLOR_BIT;
	}
	if (flags & ETextureFlagBit::TEXTURE_FLAG_DEPTH_TARGET) {
		vkFlags |= VK_IMAGE_ASPECT_DEPTH_BIT;
		vkFlags &= (~VK_IMAGE_ASPECT_COLOR_BIT);
	}
	if (flags & ETextureFlagBit::TEXTURE_FLAG_STENCIL) {
		vkFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	if (flags & ETextureFlagBit::TEXTURE_FLAG_UAV) {
		vkFlags |= VK_IMAGE_ASPECT_METADATA_BIT;
	}
	return vkFlags;
}

 VkFilter ToFilter(ESamplerFilter filter) {
	if (filter == ESamplerFilter::Bilinear || filter == ESamplerFilter::Trilinear || filter == ESamplerFilter::AnisotropicLinear) {
		return VK_FILTER_LINEAR;
	}
	return VK_FILTER_NEAREST;
}

 VkSamplerAddressMode ToAddressMode(ESamplerAddressMode mode) {
	switch (mode) {
	case ESamplerAddressMode::Wrap: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case ESamplerAddressMode::Border: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	case ESamplerAddressMode::Clamp: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case ESamplerAddressMode::Mirror: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	default: return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
	}
}

 VkShaderStageFlagBits ToVkShaderStageFlagBit(EShaderStageFlagBit type) {
	switch (type) {
	case EShaderStageFlagBit::SHADER_STAGE_COMPUTE_BIT: return VK_SHADER_STAGE_COMPUTE_BIT;
	case EShaderStageFlagBit::SHADER_STAGE_VERTEX_BIT: return VK_SHADER_STAGE_VERTEX_BIT;
	case EShaderStageFlagBit::SHADER_STAGE_GEOMETRY_BIT: return VK_SHADER_STAGE_GEOMETRY_BIT;
	case EShaderStageFlagBit::SHADER_STAGE_PIXEL_BIT: return VK_SHADER_STAGE_FRAGMENT_BIT;
	}
	return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
}

VkShaderStageFlags ToVkShaderStageFlags(EShaderStageFlags flags) {
	VkShaderStageFlags vkFlags = 0;
	if(flags & EShaderStageFlagBit::SHADER_STAGE_COMPUTE_BIT) {
		vkFlags |= VK_SHADER_STAGE_COMPUTE_BIT;
	}
	if(flags & EShaderStageFlagBit::SHADER_STAGE_VERTEX_BIT ) {
		vkFlags |= VK_SHADER_STAGE_VERTEX_BIT;
	}
	if(flags & EShaderStageFlagBit::SHADER_STAGE_GEOMETRY_BIT) {
		vkFlags |= VK_SHADER_STAGE_GEOMETRY_BIT;
	}
	if(flags & EShaderStageFlagBit::SHADER_STAGE_PIXEL_BIT) {
		vkFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
	}
	return vkFlags;
}

VkDescriptorType ToVkDescriptorType(EBindingType type) {
	switch (type) {
	case EBindingType::Texture: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	case EBindingType::Sampler: return VK_DESCRIPTOR_TYPE_SAMPLER;
	case EBindingType::TextureSampler: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	case EBindingType::StorageTexture: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	case EBindingType::UniformBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	case EBindingType::StorageBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	}
	return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}

 VkPrimitiveTopology ToPrimitiveTopology(EPrimitiveTopology topology) {
	switch (topology) {
	case EPrimitiveTopology::TriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	case EPrimitiveTopology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	case EPrimitiveTopology::PointList: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	case EPrimitiveTopology::LineList: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	case EPrimitiveTopology::LineStrip: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
	}
	return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
}

 VkPolygonMode ToPolygonMode(ERasterizerFill fill) {
	switch(fill) {
	case ERasterizerFill::Point: return VK_POLYGON_MODE_POINT;
	case ERasterizerFill::Solid: return VK_POLYGON_MODE_FILL;
	case ERasterizerFill::Wireframe: return VK_POLYGON_MODE_LINE;
	}
	return VK_POLYGON_MODE_MAX_ENUM;
}

 VkCullModeFlagBits ToVkCullMode(ERasterizerCull cull) {
	switch(cull) {
	case ERasterizerCull::Back: return VK_CULL_MODE_BACK_BIT;
	case ERasterizerCull::Front: return VK_CULL_MODE_FRONT_BIT;
	case ERasterizerCull::Null: return VK_CULL_MODE_NONE;
	}
	return VK_CULL_MODE_FLAG_BITS_MAX_ENUM;
}

 VkSampleCountFlagBits ToVkMultiSampleCount(uint8 count) {
	switch (count) {
	case 1: return VK_SAMPLE_COUNT_1_BIT;
	case 2: return VK_SAMPLE_COUNT_2_BIT;
	case 4: return VK_SAMPLE_COUNT_4_BIT;
	case 8: return VK_SAMPLE_COUNT_8_BIT;
	case 16: return VK_SAMPLE_COUNT_16_BIT;
	case 32: return VK_SAMPLE_COUNT_32_BIT;
	case 64: return VK_SAMPLE_COUNT_64_BIT;
	default: break;
	}
	LOG_ERROR("[ToVkMultiSampleCount] Invalid multi sample count: %u", count);
}

 VkCompareOp ToVkCompareOp(ECompareType type) {
	switch (type) {
	case ECompareType::Less: return VK_COMPARE_OP_LESS;
	case ECompareType::LessEqual: return VK_COMPARE_OP_LESS_OR_EQUAL;
	case ECompareType::Greater: return VK_COMPARE_OP_GREATER;
	case ECompareType::GreaterEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
	case ECompareType::Equal: return VK_COMPARE_OP_EQUAL;
	case ECompareType::NotEqual: return VK_COMPARE_OP_NOT_EQUAL;
	case ECompareType::Never: return VK_COMPARE_OP_NEVER;
	case ECompareType::Always: return VK_COMPARE_OP_ALWAYS;
	}
	return VK_COMPARE_OP_MAX_ENUM;
}

 VkStencilOp ToVkStencilOp(EStencilOp op) {
	switch(op) {
	case EStencilOp::Keep: return VK_STENCIL_OP_KEEP;
	case EStencilOp::Zero: return VK_STENCIL_OP_ZERO;
	case EStencilOp::Replace: return VK_STENCIL_OP_REPLACE;
	case EStencilOp::SaturatedIncrement: return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
	case EStencilOp::SaturatedDecrement: return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
	case EStencilOp::Invert: return VK_STENCIL_OP_INVERT;
	case EStencilOp::Increment: return VK_STENCIL_OP_INCREMENT_AND_WRAP;
	case EStencilOp::Decrement: return VK_STENCIL_OP_DECREMENT_AND_WRAP;
	}
	return VK_STENCIL_OP_MAX_ENUM;
}

 VkStencilOpState ToVkStencilOpState(const RHIDepthStencilState::StencilOpDesc& desc) {
	VkStencilOpState state;
	state.failOp = ToVkStencilOp(desc.FailOp);
	state.passOp = ToVkStencilOp(desc.PassOp);
	state.depthFailOp = ToVkStencilOp(desc.DepthFailOp);
	state.compareOp = ToVkCompareOp(desc.CompareType);
	state.compareMask = desc.ReadMask;
	state.writeMask = desc.WriteMask;
	state.reference = 0;
	return state;
}

VkPipelineRasterizationStateCreateInfo ToRasterizationStateCreateInfo(const RHIRasterizerState& desc) {

	VkPipelineRasterizationStateCreateInfo info{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, nullptr, 0 };
	info.depthClampEnable = false;
	info.rasterizerDiscardEnable = false; // todo
	info.polygonMode = ToPolygonMode(desc.FillMode);
	info.cullMode = ToVkCullMode(desc.CullMode);
	info.frontFace = desc.Clockwise ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
	info.depthBiasEnable = 0.0f != desc.DepthBiasConstant;
	if (info.depthBiasEnable) {
		info.depthBiasConstantFactor = desc.DepthBiasConstant;
		info.depthBiasSlopeFactor = desc.DepthBiasSlope;
	}
	info.lineWidth = 1.0f;
	return info;
}

VkPipelineDepthStencilStateCreateInfo ToDepthStencilStateCreateInfo(const RHIDepthStencilState& desc) {
	VkPipelineDepthStencilStateCreateInfo info{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, nullptr, 0 };
	info.depthTestEnable = ECompareType::Always != desc.DepthCompare;
	info.depthWriteEnable = desc.DepthWrite;
	info.depthCompareOp = ToVkCompareOp(desc.DepthCompare);
	info.depthBoundsTestEnable = false;
	info.stencilTestEnable = desc.StencilTest;
	if (info.stencilTestEnable) {
		info.front = ToVkStencilOpState(desc.FrontStencil);
		info.back = ToVkStencilOpState(desc.BackStencil);
	}
	return info;
}

 VkBlendOp ToVkBlendOp(EBlendOption op) {
	switch(op) {
	case EBlendOption::Add: return VK_BLEND_OP_ADD;
	case EBlendOption::Sub: return VK_BLEND_OP_SUBTRACT;
	case EBlendOption::Min: return VK_BLEND_OP_MIN;
	case EBlendOption::Max: return VK_BLEND_OP_MAX;
	case EBlendOption::ReverseSub: return VK_BLEND_OP_REVERSE_SUBTRACT;
	}
	return VK_BLEND_OP_MAX_ENUM;
}

 VkBlendFactor ToBlendFactor(EBlendFactor factor) {
	switch(factor) {
	case EBlendFactor::Zero: return VK_BLEND_FACTOR_ZERO;
	case EBlendFactor::One: return VK_BLEND_FACTOR_ONE;
	case EBlendFactor::SrcColor: return VK_BLEND_FACTOR_SRC_COLOR;
	case EBlendFactor::InverseSrcColor: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
	case EBlendFactor::DstColor: return VK_BLEND_FACTOR_DST_COLOR;
	case EBlendFactor::InverseDstColor: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
	case EBlendFactor::SrcAlpha: return VK_BLEND_FACTOR_SRC_ALPHA;
	case EBlendFactor::InverseSrcAlpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	case EBlendFactor::DstAlpha: return VK_BLEND_FACTOR_DST_ALPHA;
	case EBlendFactor::InverseDstAlpha: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
	case EBlendFactor::ConstColor: return VK_BLEND_FACTOR_CONSTANT_COLOR;
	case EBlendFactor::InverseConstColor: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
	case EBlendFactor::ConstAlpha: return VK_BLEND_FACTOR_CONSTANT_ALPHA;
	case EBlendFactor::InverseConstAlpha: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
	}
	return VK_BLEND_FACTOR_MAX_ENUM;
}

 VkPipelineColorBlendAttachmentState ToAttachmentBlendState(const RHIBlendState& blendState) {
	VkPipelineColorBlendAttachmentState state;
	state.blendEnable = blendState.Enable;
	state.srcColorBlendFactor = ToBlendFactor(blendState.ColorSrc);
	state.dstColorBlendFactor = ToBlendFactor(blendState.ColorDst);
	state.colorBlendOp = ToVkBlendOp(blendState.ColorBlendOp);
	state.srcAlphaBlendFactor = ToBlendFactor(blendState.AlphaSrc);
	state.dstAlphaBlendFactor = ToBlendFactor(blendState.AlphaDst);
	state.alphaBlendOp = ToVkBlendOp(blendState.AlphaBlendOp);
	state.colorWriteMask = blendState.ColorWriteMask;
	return state;
}

 VkAttachmentLoadOp ToVkAttachmentLoadOp(ERTLoadOption op) {
	switch(op) {
	case ERTLoadOption::Clear: return VK_ATTACHMENT_LOAD_OP_CLEAR;
	case ERTLoadOption::Load: return VK_ATTACHMENT_LOAD_OP_LOAD;
	case ERTLoadOption::NoAction: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	}
	return VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
}

 VkAttachmentStoreOp ToVkAttachmentStoreOp(ERTStoreOption op) {
	switch(op) {
	case ERTStoreOption::EStore:
		return VK_ATTACHMENT_STORE_OP_STORE;
	case ERTStoreOption::ENoAction:
		return VK_ATTACHMENT_STORE_OP_DONT_CARE;
	}
	return VK_ATTACHMENT_STORE_OP_MAX_ENUM;
}

 VkImageLayout ToImageLayout(EResourceState state) {
	switch(state) {
	case EResourceState::ColorTarget:
		return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	case EResourceState::DepthStencil:
		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	case EResourceState::ShaderResourceView:
		return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	case EResourceState::UnorderedAccessView:
		return VK_IMAGE_LAYOUT_GENERAL;
	case EResourceState::TransferSrc:
		return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	case EResourceState::TransferDst:
		return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	case EResourceState::Present:
		return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}
	return VK_IMAGE_LAYOUT_UNDEFINED;
}

uint32 ConvertImageArraySize(const RHITextureDesc& desc) {
	switch (desc.Dimension) {
	case ETextureDimension::Tex2D:
	case ETextureDimension::Tex3D:
		return 1;
	case ETextureDimension::Tex2DArray:
		return desc.ArraySize;
	case ETextureDimension::TexCube:
		return 6;
	case ETextureDimension::TexCubeArray:
		return 6 * desc.ArraySize;
	default:
		return 0;
	}
}
