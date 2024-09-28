#include "VulkanResources.h"
#include "VulkanCommand.h"
#include "VulkanRHI.h"
#include "VulkanConverter.h"
#include "VulkanPipeline.h"
#include "VulkanDevice.h"
#include "Math/Public/Math.h"

VulkanBuffer::VulkanBuffer(const RHIBufferDesc& desc, VulkanDevice* device) : RHIBuffer(desc), m_Device(device) {
	CHECK(m_Device->GetMemoryAllocator()->AllocateBufferMemory(m_Allocation, desc.ByteSize, ToBufferUsage(desc.Flags), ToBufferMemoryProperty(desc.Flags)));
}

VulkanBuffer::~VulkanBuffer() {
	m_Device->GetMemoryAllocator()->FreeBufferMemory(m_Allocation);
}

void VulkanBuffer::SetName(const char* name) {
	VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_BUFFER, GetBuffer(), name);
}

void VulkanBuffer::UpdateData(const void* data, uint32 byteSize, uint32 offset) {
	void* mappedData = ((uint8*)m_Allocation.Map() + offset);
	memcpy(mappedData, data, byteSize);
	m_Allocation.Unmap();
}

void VulkanBufferImpl::UpdateData(const void* data, uint32 byteSize, uint32 offset) {
	const EBufferFlags bufferFlags = GetDesc().Flags;
	if(EnumHasAnyFlags(bufferFlags, EBufferFlags::Uniform)) {
		VulkanBuffer::UpdateData(data, byteSize, offset);
	}
	else {
		ASSERT(bufferFlags & EBufferFlags::CopyDst, "[HIVulkanBufferWithMem::UpdateData] Buffer is not used with Copy Dst!");
		VulkanStagingBuffer* staging = m_Device->GetUploader()->AcquireBuffer(byteSize);
		staging->UpdateData(data, byteSize, offset);
		auto cmd = m_Device->GetCommandContext()->GetUploadCmd(EQueueType::Transfer);
		cmd->CopyBufferToBuffer(staging, this, 0, offset, byteSize);
	}
}

VkImageView VulkanRHITexture::GetDefaultView() {
	return GetView(GetDesc().GetDefaultSubRes());
}

VulkanTextureImpl::VulkanTextureImpl(const RHITextureDesc& desc, VulkanDevice* device): VulkanRHITexture(desc), m_Device(device), m_Image(nullptr) {
	VkImageCreateInfo imageInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, nullptr};
	imageInfo.flags = ToImageCreateFlags(desc.Dimension);
	imageInfo.imageType = ToImageType(desc.Dimension);
	imageInfo.format = ToVkFormat(desc.Format);
	imageInfo.extent = { desc.Width, desc.Height, desc.Depth};
	imageInfo.mipLevels = desc.MipSize;
	imageInfo.arrayLayers = desc.ArraySize * GetImagePerLayerSize(desc.Dimension);
	imageInfo.samples = ToVkMultiSampleCount(desc.Samples);
	imageInfo.usage = ToImageUsage(desc.Flags);
	VK_CHECK(vkCreateImage(m_Device->GetDevice(), &imageInfo, nullptr, &m_Image));
	// memory
	VkMemoryPropertyFlags memoryProperty = ToImageMemoryProperty(desc.Flags);
	CHECK(m_Device->GetMemoryAllocator()->AllocateImageMemory(m_Allocation, m_Image, memoryProperty));
	// Reserve view size
	m_Views.Resize(GetDesc().ArraySize);
}

VulkanTextureImpl::~VulkanTextureImpl() {
	for(auto& subResViews: m_Views) {
		for(auto& subResView: subResViews) {
			vkDestroyImageView(m_Device->GetDevice(), subResView.View, nullptr);
		}
	}
	vkDestroyImage(m_Device->GetDevice(), m_Image, nullptr);
	m_Device->GetMemoryAllocator()->FreeImageMemory(m_Allocation);
}

VkImageView VulkanTextureImpl::GetView(RHITextureSubRes subRes) {
	GetDesc().LimitSubRes(subRes);
	auto& subResViews = m_Views[subRes.ArrayIndex];
	for(auto& subResView: subResViews) {
		if(subResView.SubRes == subRes) {
			return subResView.View;
		}
	}
	VkImageView imageView = CreateView(subRes);
	subResViews.PushBack({ subRes, imageView });
	return imageView;
}

void VulkanTextureImpl::SetName(const char* name) {
	VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_IMAGE, m_Image, name);
}

void VulkanTextureImpl::UpdateData(const void* data, uint32 byteSize, RHITextureSubRes subRes, IOffset3D offset) {
	ASSERT(GetDesc().Flags & ETextureFlags::CopyDst, "");
	VulkanStagingBuffer* staging = m_Device->GetUploader()->AcquireBuffer(byteSize);
	staging->UpdateData(data, byteSize, 0);
	auto cmd = m_Device->GetCommandContext()->GetUploadCmd(EQueueType::Graphics);
	cmd->TransitionTextureState(this, EResourceState::Unknown, EResourceState::TransferDst, subRes);
	cmd->CopyBufferToTexture(staging, this, subRes, offset);
	cmd->TransitionTextureState(this, EResourceState::TransferDst, EResourceState::ShaderResourceView, subRes);
}

VkImageView VulkanTextureImpl::CreateView(RHITextureSubRes subRes) {
	ASSERT(IsDimensionCompatible(GetDesc().Dimension, subRes.Dimension), "[VulkanRHITexture::CreateView] Dimension is not compatible!");
	VkImageViewCreateInfo imageViewCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, nullptr, 0 };
	imageViewCreateInfo.image = m_Image;
	imageViewCreateInfo.viewType = ToImageViewType(subRes.Dimension);
	imageViewCreateInfo.format = ToVkFormat(m_Desc.Format);
	ToImageSubResourceRange(subRes, imageViewCreateInfo.subresourceRange);
	VkImageView imageView;
	vkCreateImageView(m_Device->GetDevice(), &imageViewCreateInfo, nullptr, &imageView);
	return imageView;
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

VulkanRHIShader::VulkanRHIShader(EShaderStageFlags type, const char* code, uint32 codeSize, const XString& entryFunc, VulkanDevice* device): RHIShader(type), m_Device(device) {
	VkShaderModuleCreateInfo shaderInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr, 0, (size_t)codeSize, reinterpret_cast<const uint32*>(code)};
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