#include "VulkanResources.h"
#include "VulkanCommand.h"
#include "VulkanRHI.h"
#include "VulkanConverter.h"
#include "VulkanPipeline.h"
#include "VulkanDevice.h"
#include "VulkanUploader.h"
#include "Math/Public/Math.h"

VulkanRHIBuffer::VulkanRHIBuffer(const RHIBufferDesc& desc, VulkanDevice* device, BufferAllocation&& alloc):
RHIBuffer(desc), m_Device(device), m_Allocation(std::forward<BufferAllocation>(alloc)) {}

VulkanRHIBuffer::~VulkanRHIBuffer() {
	m_Device->GetMemoryMgr()->FreeBufferMemory(m_Allocation);
}

void VulkanRHIBuffer::SetName(const char* name) {
	VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_BUFFER, GetBuffer(), name);
}

void VulkanRHIBuffer::UpdateData(const void* data, uint32 byteSize, uint32 offset) {
	const EBufferFlags bufferFlags = GetDesc().Flags;
	if(bufferFlags & EBufferFlagBit::BUFFER_FLAG_UNIFORM) {
		void* mappedData = ((uint8*)m_Allocation.Map() + offset);
		memcpy(mappedData, data, byteSize);
		m_Allocation.Unmap();
	}
	else {
		ASSERT(bufferFlags & EBufferFlagBit::BUFFER_FLAG_COPY_DST, "[HIVulkanBufferWithMem::UpdateData] Buffer is not used with Copy Dst!");
		VulkanStagingBuffer* staging = m_Device->GetUploader()->AcquireBuffer(byteSize);
		void* mappedPointer = staging->Map();
		memcpy(mappedPointer, data, byteSize);
		staging->Unmap();
		auto cmd = m_Device->GetCommandContext()->GetUploadCmd();
		// TODO replace with vulkan functions
		cmd->CopyBufferToBuffer(staging, this, 0, offset, byteSize);
	}
}

VulkanRHITexture::VulkanRHITexture(const RHITextureDesc& desc, VulkanDevice* device, VkImage image, ImageAllocation&& alloc):
	RHITexture(desc),
	m_Device(device),
	m_Image(image),
	m_Allocation(MoveTemp(alloc)),
	m_IsImageOwner(true){
	// reserve the views
	const uint32 viewCount = ConvertImageArraySize(m_Desc) * m_Desc.MipSize;
	m_2DViewIndices.Resize(viewCount, VK_INVALID_INDEX);
	if (desc.Dimension == ETextureDimension::TexCube) {
		m_CubeViewIndices.Resize(m_Desc.MipSize, VK_INVALID_INDEX);
	}
	m_DefaultViewIndex = VK_INVALID_INDEX;
}

VulkanRHITexture::~VulkanRHITexture() {
	for(VkImageView view: m_AllViews) {
		vkDestroyImageView(m_Device->GetDevice(), view, nullptr);
	}
	if (m_IsImageOwner && m_Image) {
		vkDestroyImage(m_Device->GetDevice(), m_Image, nullptr);
		m_Device->GetMemoryMgr()->FreeImageMemory(m_Allocation);
	}
}

VkImageView VulkanRHITexture::GetView(ETextureSRVType type, RHITextureSubDesc sub) {
	switch (type) {
	case ETextureSRVType::Default: return GetDefaultView();
	case ETextureSRVType::Texture2D: return Get2DView(sub.MipIndex, sub.ArrayIndex);
	case ETextureSRVType::CubeMap: return GetCubeView(sub.MipIndex, sub.ArrayIndex);
	default: LOG_ERROR("[VulkanRHITexture::GetView] Vinalid src type!");
	}
}

VkImageView VulkanRHITexture::GetDefaultView() {
	if(VK_INVALID_INDEX == m_DefaultViewIndex) {
		m_DefaultViewIndex = CreateView(ToImageViewType(m_Desc.Dimension), ToImageAspectFlags(m_Desc.Flags), 0, m_Desc.MipSize, 0, m_Desc.ArraySize);
	}
	return m_AllViews[m_DefaultViewIndex];
}

VkImageView VulkanRHITexture::Get2DView(uint8 mipIndex, uint32 arrayIndex) {
	const uint32 idx = mipIndex * ConvertImageArraySize(m_Desc) + arrayIndex;
	CHECK(idx < m_2DViewIndices.Size());
	if(m_2DViewIndices[idx] == VK_INVALID_INDEX) {
		m_2DViewIndices[idx] = CreateView(VK_IMAGE_VIEW_TYPE_2D, ToImageAspectFlags(m_Desc.Flags), mipIndex, 1, arrayIndex, 1);
	}
	return m_AllViews[m_2DViewIndices[idx]];
}

VkImageView VulkanRHITexture::GetCubeView(uint8 mipIndex, uint32 arrayIndex) {
	ASSERT(m_Desc.Dimension == ETextureDimension::TexCube || m_Desc.Dimension == ETextureDimension::TexCubeArray, "Only TexCube or TexCubeArray has CubeView!");
	const uint32 idx = mipIndex * m_Desc.ArraySize + arrayIndex * 6;
	CHECK(idx < m_CubeViewIndices.Size());
	if (m_CubeViewIndices[idx] == VK_INVALID_INDEX) {
		m_CubeViewIndices[idx] = CreateView(VK_IMAGE_VIEW_TYPE_CUBE, ToImageAspectFlags(m_Desc.Flags), mipIndex, 1, arrayIndex, 6);
	}
	return m_AllViews[m_CubeViewIndices[idx]];
}

void VulkanRHITexture::SetName(const char* name) {
	VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_IMAGE, m_Image, name);
}

void VulkanRHITexture::UpdateData(uint32 byteSize, const void* data, RHITextureOffset offset) {
	ASSERT(GetDesc().Flags & ETextureFlagBit::TEXTURE_FLAG_CPY_DST, "");
	VulkanStagingBuffer* staging = m_Device->GetUploader()->AcquireBuffer(byteSize);
	void* mappedPointer = staging->Map();
	memcpy(mappedPointer, data, byteSize);
	staging->Unmap();
	// calc layer count by byteSize
	uint32 sliceSize = m_Desc.Width * m_Desc.Height * GetPixelByteSize();
	uint32 arrayCount = (uint32)Math::Ceil<float>((float)byteSize / (float)sliceSize);
	CHECK(arrayCount <= m_Desc.ArraySize);
	// TODO replace with vulkan functions
	auto cmd = m_Device->GetCommandContext()->GetUploadCmd();
	RHITextureSubDesc desc{ offset.MipLevel, 1, offset.ArrayLayer, (uint8)arrayCount};
	cmd->TransitionTextureState(this, EResourceState::Unknown, EResourceState::TransferDst, desc);
	cmd->CopyBufferToTexture(staging, this, offset.MipLevel, offset.ArrayLayer, arrayCount);
	cmd->TransitionTextureState(this, EResourceState::TransferDst, EResourceState::ShaderResourceView, desc);
}

inline bool CheckViewable(ETextureDimension dimension, VkImageViewType type) {
	if (dimension == ETextureDimension::Tex2D) {
		return type == VK_IMAGE_VIEW_TYPE_2D;
	}
	if (dimension == ETextureDimension::Tex2DArray) {
		return type == VK_IMAGE_VIEW_TYPE_2D ||
			type == VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	}
	if (dimension == ETextureDimension::TexCube) {
		return type == VK_IMAGE_VIEW_TYPE_2D ||
			type == VK_IMAGE_VIEW_TYPE_CUBE;
	}
	if (dimension == ETextureDimension::TexCubeArray) {
		return type == VK_IMAGE_VIEW_TYPE_2D ||
			type == VK_IMAGE_VIEW_TYPE_CUBE ||
			type == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
	}
	if(dimension == ETextureDimension::Tex3D) {
		return type == VK_IMAGE_VIEW_TYPE_3D;
	}
	return false;
}

inline uint32 ConvertVulkanLayerCount(VkImageViewType type, uint32 layerCount) {

	switch (type) {
	case VK_IMAGE_VIEW_TYPE_2D:
	case VK_IMAGE_VIEW_TYPE_1D:
	case VK_IMAGE_VIEW_TYPE_3D:
		return 1;
	case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
	case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
		return layerCount;
	case VK_IMAGE_VIEW_TYPE_CUBE:
		return 6;
	case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
		return 6 * layerCount;
	default:
		LOG_ERROR("Invalid view type!");
		return 1;
	}
}

uint32 VulkanRHITexture::CreateView(VkImageViewType type, VkImageAspectFlags aspectFlags, uint8 mipIndex, uint8 mipCount, uint32 arrayIndex, uint32 arrayCount) {
	ASSERT(CheckViewable(m_Desc.Dimension, type), "Type is not viewable!");
	arrayCount = ConvertVulkanLayerCount(type, arrayCount);
	uint32 index = m_AllViews.Size();
	// correct layer count
	VkImageViewCreateInfo imageViewCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, nullptr, 0 };
	imageViewCreateInfo.image = m_Image;
	imageViewCreateInfo.viewType = type;
	imageViewCreateInfo.format = ToVkFormat(m_Desc.Format);
	imageViewCreateInfo.subresourceRange.aspectMask = aspectFlags;
	imageViewCreateInfo.subresourceRange.baseMipLevel = mipIndex;
	imageViewCreateInfo.subresourceRange.levelCount = mipCount;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = arrayIndex;
	imageViewCreateInfo.subresourceRange.layerCount = arrayCount;
	vkCreateImageView(m_Device->GetDevice(), &imageViewCreateInfo, nullptr, &m_AllViews.EmplaceBack());
	return index;
}

VulkanDepthStencilTexture::VulkanDepthStencilTexture(const RHITextureDesc& desc, VulkanDevice* device, VkImage image, ImageAllocation&& allocation):
VulkanRHITexture(desc, device, image, MoveTemp(allocation)),
m_DepthViewIndex(VK_INVALID_INDEX),
m_StencilViewIndex(VK_INVALID_INDEX){}

VkImageView VulkanDepthStencilTexture::GetView(ETextureSRVType type, RHITextureSubDesc sub) {
	if(ETextureSRVType::Depth == type) {
		return GetDepthView();
	}
	if(ETextureSRVType::Stencil == type) {
		return GetStencilView();
	}
	return VulkanRHITexture::GetView(type, sub);
}

VkImageView VulkanDepthStencilTexture::GetDepthView() {
	CHECK(m_Desc.Flags & ETextureFlagBit::TEXTURE_FLAG_DEPTH_TARGET);
	if(VK_INVALID_INDEX == m_DepthViewIndex) {
		m_DepthViewIndex = CreateView(ToImageViewType(m_Desc.Dimension), VK_IMAGE_ASPECT_DEPTH_BIT, 0, m_Desc.MipSize, 0, m_Desc.ArraySize);
	}
	return m_AllViews[m_DepthViewIndex];
}

VkImageView VulkanDepthStencilTexture::GetStencilView() {
	CHECK(m_Desc.Flags & ETextureFlagBit::TEXTURE_FLAG_STENCIL);
	if (VK_INVALID_INDEX == m_StencilViewIndex) {
		m_StencilViewIndex = CreateView(ToImageViewType(m_Desc.Dimension), VK_IMAGE_ASPECT_STENCIL_BIT, 0, m_Desc.MipSize, 0, m_Desc.ArraySize);
	}
	return m_AllViews[m_StencilViewIndex];
}

VkImageView VulkanDepthStencilTexture::GetDepthStencilView() {
	return GetDefaultView();
}

VulkanRHISampler::VulkanRHISampler(const RHISamplerDesc& desc, VulkanDevice* device, VkSampler sampler): RHISampler(desc), m_Device(device), m_Sampler(sampler) {}

VulkanRHISampler::~VulkanRHISampler() {
	vkDestroySampler(m_Device->GetDevice(), m_Sampler, nullptr);
}

void VulkanRHISampler::SetName(const char* name) {
	VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_SAMPLER, m_Sampler, name);
}

VulkanRHIFence::VulkanRHIFence(VkFence fence, VulkanDevice* device): m_Handle(fence), m_Device(device) {
}


VulkanRHIFence::~VulkanRHIFence() {
	vkDestroyFence(m_Device->GetDevice(), m_Handle, nullptr);
}

void VulkanRHIFence::Wait() {
	vkWaitForFences(m_Device->GetDevice(), 1, &m_Handle, 1, VK_WAIT_MAX);
}

void VulkanRHIFence::Reset() {
	vkResetFences(m_Device->GetDevice(), 1, &m_Handle);
}

void VulkanRHIFence::SetName(const char* name) {
	VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_FENCE, m_Handle, name);
}

VulkanRHIShader::VulkanRHIShader(EShaderStageFlagBit type, const char* code, size_t codeSize, const XString& entryFunc, VulkanDevice* device): RHIShader(type), m_Device(device) {
	VkShaderModuleCreateInfo shaderInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr, 0, codeSize, reinterpret_cast<const uint32*>(code)};
	vkCreateShaderModule(m_Device->GetDevice(), &shaderInfo, nullptr, &m_ShaderModule);
	m_EntryName = entryFunc;
}

VulkanRHIShader::~VulkanRHIShader() {
	vkDestroyShaderModule(m_Device->GetDevice(), m_ShaderModule, nullptr);
}

void VulkanRHIShader::SetName(const char* name) {
	VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_SHADER_MODULE, m_ShaderModule, name);
}

VkPipelineShaderStageCreateInfo VulkanRHIShader::GetShaderStageInfo() const {
	VkPipelineShaderStageCreateInfo info{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0 };
	info.stage = ToVkShaderStageFlagBit(m_Type);
	info.module = m_ShaderModule;
	info.pName = m_EntryName.data();
	info.pSpecializationInfo = nullptr;
	return info;
}