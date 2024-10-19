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
	if (EnumHasAnyFlags(flags, EBufferFlags::CopySrc)) {
		usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	}
	if (EnumHasAnyFlags(flags, EBufferFlags::CopyDst)) {
		usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}
	if (EnumHasAnyFlags(flags, EBufferFlags::Uniform)) {
		usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	}
	if (EnumHasAnyFlags(flags, EBufferFlags::Index)) {
		usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	}
	if (EnumHasAnyFlags(flags, EBufferFlags::Vertex)) {
		usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	}
	if (EnumHasAnyFlags(flags, EBufferFlags::Storage)) {
		usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	}
	if (EnumHasAnyFlags(flags, EBufferFlags::IndirectDraw)) {
		usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
	}
	return usage;
}

 VkMemoryPropertyFlags ToBufferMemoryProperty(EBufferFlags flags) {
	if (EnumHasAnyFlags(flags, EBufferFlags::CopySrc | EBufferFlags::Uniform | EBufferFlags::Storage)) {
		return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	}
	if (EnumHasAnyFlags(flags, EBufferFlags::Index | EBufferFlags::Vertex)) {
		return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	}
	return 0;
}

 VkMemoryPropertyFlags ToImageMemoryProperty(ETextureFlags flags) {
	//if (flags & (ETextureFlags::TEXTURE_USAGE_CPY_SRC)) {
	//	return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	//}
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
	if (EnumHasAnyFlags(flags, ETextureFlags::DepthStencilTarget)) {
		vkFlags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	}
	if (EnumHasAnyFlags(flags, ETextureFlags::ColorTarget)) {
		vkFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}
	if (EnumHasAnyFlags(flags, ETextureFlags::SRV)) {
		vkFlags |= VK_IMAGE_USAGE_SAMPLED_BIT;
	}
	if (EnumHasAnyFlags(flags, ETextureFlags::CopyDst)) {
		vkFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}
	if (EnumHasAnyFlags(flags, ETextureFlags::CopySrc)) {
		vkFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}
	if (EnumHasAnyFlags(flags, ETextureFlags::UAV)) {
		vkFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
	}
	if (EnumHasAnyFlags(flags, ETextureFlags::Present)) {
	}
	return vkFlags;
}

VkImageAspectFlags ToImageAspectFlags(ETextureViewFlags flags) {
	VkImageAspectFlags vkFlags = 0;
	if (EnumHasAnyFlags(flags, ETextureViewFlags::Color)) {
		vkFlags |= VK_IMAGE_ASPECT_COLOR_BIT;
	}
	if (EnumHasAnyFlags(flags, ETextureViewFlags::Depth)) {
		vkFlags |= VK_IMAGE_ASPECT_DEPTH_BIT;
		vkFlags &= (~VK_IMAGE_ASPECT_COLOR_BIT);
	}
	if (EnumHasAnyFlags(flags, ETextureViewFlags::Stencil)) {
		vkFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	if (EnumHasAnyFlags(flags, ETextureViewFlags::MetaData)) {
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

 VkShaderStageFlagBits ToVkShaderStageFlagBit(EShaderStageFlags type) {
	switch (type) {
	case EShaderStageFlags::Compute: return VK_SHADER_STAGE_COMPUTE_BIT;
	case EShaderStageFlags::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
	case EShaderStageFlags::Geometry: return VK_SHADER_STAGE_GEOMETRY_BIT;
	case EShaderStageFlags::Pixel: return VK_SHADER_STAGE_FRAGMENT_BIT;
	}
	return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
}

VkShaderStageFlags ToVkShaderStageFlags(EShaderStageFlags flags) {
	VkShaderStageFlags vkFlags = 0;
	if(EnumHasAnyFlags(flags, EShaderStageFlags::Compute)) {
		vkFlags |= VK_SHADER_STAGE_COMPUTE_BIT;
	}
	if(EnumHasAnyFlags(flags, EShaderStageFlags::Vertex)) {
		vkFlags |= VK_SHADER_STAGE_VERTEX_BIT;
	}
	if(EnumHasAnyFlags(flags, EShaderStageFlags::Geometry)) {
		vkFlags |= VK_SHADER_STAGE_GEOMETRY_BIT;
	}
	if(EnumHasAnyFlags(flags, EShaderStageFlags::Pixel)) {
		vkFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
	}
	return vkFlags;
}

VkDescriptorType ToVkDescriptorType(EBindingType type) {
	switch (type) {
	case EBindingType::Texture: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	case EBindingType::Sampler: return VK_DESCRIPTOR_TYPE_SAMPLER;
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
	state.reference = 0;
	return state;
}

VkPipelineRasterizationStateCreateInfo ToRasterizationStateCreateInfo(const RHIRasterizerState& desc) {

	VkPipelineRasterizationStateCreateInfo info{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, nullptr, 0 };
	info.depthClampEnable = false;
	info.rasterizerDiscardEnable = false; // todo
	info.polygonMode = ToPolygonMode(desc.FillMode);
	info.cullMode = ToVkCullMode(desc.CullMode);
	info.frontFace = desc.FrontClockwise ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
	info.depthBiasEnable = 0.0f != desc.DepthBiasConstant;
	if (info.depthBiasEnable) {
		info.depthBiasConstantFactor = desc.DepthBiasConstant;
		info.depthBiasSlopeFactor = desc.DepthBiasSlope;
		info.depthBiasClamp = desc.DepthBiasClamp;
	}
	info.lineWidth = 1.0f;
	return info;
}

VkPipelineDepthStencilStateCreateInfo ToDepthStencilStateCreateInfo(const RHIDepthStencilState& desc) {
	VkPipelineDepthStencilStateCreateInfo info{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, nullptr, 0 };
	info.depthTestEnable = desc.DepthTest;
	info.depthWriteEnable = desc.DepthWrite;
	info.depthCompareOp = ToVkCompareOp(desc.DepthCompare);
	info.depthBoundsTestEnable = false;
	info.stencilTestEnable = desc.StencilTest;
	if (info.stencilTestEnable) {
		info.front = ToVkStencilOpState(desc.FrontStencil);
		info.front.compareMask = desc.StencilReadMask;
		info.front.writeMask = desc.StencilReadMask;
		info.back = ToVkStencilOpState(desc.BackStencil);
		info.back.compareMask = desc.StencilReadMask;
		info.back.writeMask = desc.StencilReadMask;
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

VkColorComponentFlags ToVkColorComponentFlags(EColorComponentFlags flags) {
	VkColorComponentFlags dstFlags = 0;
	if (EnumHasAnyFlags(flags, EColorComponentFlags::R)) {
		dstFlags |= VK_COLOR_COMPONENT_R_BIT;
	}
	if (EnumHasAnyFlags(flags, EColorComponentFlags::G)) {
		dstFlags |= VK_COLOR_COMPONENT_G_BIT;
	}
	if (EnumHasAnyFlags(flags, EColorComponentFlags::B)) {
		dstFlags |= VK_COLOR_COMPONENT_B_BIT;
	}
	if (EnumHasAnyFlags(flags, EColorComponentFlags::A)) {
		dstFlags |= VK_COLOR_COMPONENT_A_BIT;
	}
	return dstFlags;
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
	state.colorWriteMask = ToVkColorComponentFlags(blendState.ColorWriteMask);
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

VkIndexType ToIndexType(ERHIFormat format) {
	switch (format) {
	case ERHIFormat::R16_UINT: return VK_INDEX_TYPE_UINT16;
	case ERHIFormat::R32_UINT: return VK_INDEX_TYPE_UINT32;
	case ERHIFormat::R8_UINT:  return VK_INDEX_TYPE_UINT8_KHR;
	default: return VK_INDEX_TYPE_NONE_KHR;
	}
}

VkLogicOp ToVkLogicOp(ELogicOp op) {
	switch(op) {
	case ELogicOp::Clear: return VK_LOGIC_OP_CLEAR;
	case ELogicOp::And: return VK_LOGIC_OP_AND;
	case ELogicOp::AndReverse: return VK_LOGIC_OP_AND_REVERSE;
	case ELogicOp::Copy: return VK_LOGIC_OP_COPY;
	case ELogicOp::AndInverted : return VK_LOGIC_OP_AND_INVERTED;
	case ELogicOp::NoOp : return VK_LOGIC_OP_NO_OP;
	case ELogicOp::XOr: return VK_LOGIC_OP_XOR;
	case ELogicOp::Or: return VK_LOGIC_OP_OR;
	case ELogicOp::NOr: return VK_LOGIC_OP_NOR;
	case ELogicOp::Equivalent: return VK_LOGIC_OP_EQUIVALENT;
	case ELogicOp::Invert: return VK_LOGIC_OP_INVERT;
	case ELogicOp::OrReverse: return VK_LOGIC_OP_OR_REVERSE;
	case ELogicOp::CopyInverted: return VK_LOGIC_OP_COPY_INVERTED;
	case ELogicOp::OrInverted: return VK_LOGIC_OP_OR_INVERTED;
	case ELogicOp::NAnd: return VK_LOGIC_OP_NAND;
	case ELogicOp::Set: return VK_LOGIC_OP_SET;
	default: return VK_LOGIC_OP_MAX_ENUM;
	}
}

void ToImageSubResourceLayers(RHITextureSubRes subRes, VkImageSubresourceLayers& out) {
	out.aspectMask = ToImageAspectFlags(subRes.ViewFlags);
	out.mipLevel = subRes.MipIndex;
	const uint32 perLayerSize = GetTextureDimension2DSize(subRes.Dimension);
	out.baseArrayLayer = subRes.ArrayIndex * perLayerSize;
	out.layerCount = subRes.ArraySize * perLayerSize;
}

void ToImageSubResourceRange(RHITextureSubRes subRes, VkImageSubresourceRange& out) {
	out.aspectMask = ToImageAspectFlags(subRes.ViewFlags);
	out.baseMipLevel = subRes.MipIndex;
	out.levelCount = subRes.MipSize;
	const uint32 perLayerSize = GetTextureDimension2DSize(subRes.Dimension);
	out.baseArrayLayer = subRes.ArrayIndex * perLayerSize;
	out.layerCount = subRes.ArraySize * perLayerSize;
}

VkPipelineBindPoint ToVkPipelineBindPoint(EPipelineType type) {
	switch (type) {
	case EPipelineType::Graphics: return VK_PIPELINE_BIND_POINT_GRAPHICS;
	case EPipelineType::Compute: return VK_PIPELINE_BIND_POINT_COMPUTE;
	default: return VK_PIPELINE_BIND_POINT_MAX_ENUM;
	}
}
