#include "Math/Public/MathBase.h"
#include "VulkanRHI.h"
#include "VulkanConverter.h"
#include "VulkanDevice.h"
#include "VulkanMemory.h"
#include "VulkanCommand.h"
#include "VulkanDescriptorSet.h"
#include "VulkanSwapchain.h"
#include "VulkanUploader.h"

#define RETURN_RHI_PTR(cls, hd)\
cls##Vk* cls##Ptr = new cls##Vk;\
cls##Ptr->handle = hd; \
return reinterpret_cast<cls*>(cls##Ptr)

static constexpr ERHIFormat SWAPCHAIN_FORMAT{ ERHIFormat::B8G8R8A8_SRGB };
static constexpr uint32 MIN_API_VERSION{ VK_VERSION_1_2 };

VulkanRHI::VulkanRHI(const RHIInitDesc& desc) {

	// initialize context
	m_Context.Reset(new VulkanContext(desc.EnableDebug, MIN_API_VERSION));
	// Pick GPU
	TVector<const char*> extensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	VkPhysicalDevice physicalDevice = m_Context->PickPhysicalDevice();
	ASSERT(physicalDevice != VK_NULL_HANDLE, "Failed to pick GPU!");
	// Create logic device
	m_Device.Reset(new VulkanDevice(m_Context.Get(), physicalDevice));
	// Create viewport
	m_Swapchain.Reset(new VulkanSwapchain(m_Context.Get(), m_Device.Get(), desc.WindowHandle, desc.WindowSize));

	LOG_INFO("RHI: Vulkan initialized successfully!");
}

VulkanRHI::~VulkanRHI() {
	//vma
	m_Device.Reset();
}

void VulkanRHI::Update() {
	m_Device->GetMemoryMgr()->Update();
	m_Device->GetDescriptorMgr()->Update();
	m_Device->GetUploader()->Update();
	Present();
}

ERHIFormat VulkanRHI::GetDepthFormat() {
	switch (m_Device->GetFormats().DepthFormat) {
	case(VK_FORMAT_D32_SFLOAT):
		return ERHIFormat::D32_SFLOAT;
	case(VK_FORMAT_D24_UNORM_S8_UINT):
		return ERHIFormat::D24_UNORM_S8_UINT;
	case(VK_FORMAT_D16_UNORM_S8_UINT):
		return ERHIFormat::D16_UNORM;
	default:
		return ERHIFormat::Undefined;
	}
}

USize2D VulkanRHI::GetRenderArea() {
	return m_Swapchain->GetSize();
}

const VulkanContext* VulkanRHI::GetContext() {
	return m_Context.Get();
}

VulkanDevice* VulkanRHI::GetDevice() {
	return m_Device.Get();
}

VulkanSwapchain* VulkanRHI::GetSwapchain() {
	return m_Swapchain.Get();
}

RHIBufferPtr VulkanRHI::CreateBuffer(const RHIBufferDesc& desc, void* defaultData) {
	VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, nullptr, 0 };
	bufferInfo.usage = ToBufferUsage(desc.Flags);
	bufferInfo.size = desc.ByteSize;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VkBuffer buffer;
	if (VK_SUCCESS != vkCreateBuffer(m_Device->GetDevice(), &bufferInfo, nullptr, &buffer)) {
		return nullptr;
	}
	VkMemoryPropertyFlags memoryProperty = ToBufferMemoryProperty(desc.Flags);
	BufferAllocation alloc;
	if(m_Device->GetMemoryMgr()->AllocateBufferMemory(alloc, desc.ByteSize, ToBufferUsage(desc.Flags), memoryProperty)) {
		return new VulkanRHIBuffer(desc, m_Device.Get(), MoveTemp(alloc));
	}
	return nullptr;
}

RHITexturePtr VulkanRHI::CreateTexture(const RHITextureDesc& desc, void* defaultData) {
	VkImageCreateInfo imageInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, nullptr};
	imageInfo.flags = ToImageCreateFlags(desc.Dimension);
	imageInfo.imageType = ToImageType(desc.Dimension);
	imageInfo.format = ToVkFormat(desc.Format);
	imageInfo.extent = { desc.Size.w, desc.Size.h, desc.Size.h };
	imageInfo.mipLevels = desc.NumMips;
	if(desc.Dimension == ETextureDimension::TexCube) {
		imageInfo.arrayLayers = 6;
	}
	else {
		imageInfo.arrayLayers = desc.ArraySize;
	}
	imageInfo.samples = (VkSampleCountFlagBits)desc.Samples;
	imageInfo.usage = ToImageUsage(desc.Flags);
	VkImage imageHandle;
	if(VK_SUCCESS != vkCreateImage(m_Device->GetDevice(), &imageInfo, nullptr, &imageHandle)) {
		return nullptr;
	}

	
	// memory
	ImageAllocation alloc;
	VkMemoryPropertyFlags memoryProperty = ToImageMemoryProperty(desc.Flags);
	if(!m_Device->GetMemoryMgr()->AllocateImageMemory(alloc, imageHandle, memoryProperty)) {
		vkDestroyImage(m_Device->GetDevice(), imageHandle, nullptr);
		return nullptr;
	}

	// view
	VkImageViewCreateInfo imageViewCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, nullptr, 0 };
	imageViewCreateInfo.image = imageHandle;
	imageViewCreateInfo.viewType = ToImageViewType(desc.Dimension);
	imageViewCreateInfo.format = imageInfo.format;
	imageViewCreateInfo.subresourceRange.aspectMask = ToImageAspectFlags(desc.Flags);
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = desc.NumMips;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = desc.ArraySize;
	VkImageView viewHandle;
	vkCreateImageView(m_Device->GetDevice(), &imageViewCreateInfo, nullptr, &viewHandle);

	return new VulkanRHITexture(desc, GetDevice(), imageHandle, viewHandle, MoveTemp(alloc));
}

RHISamplerPtr VulkanRHI::CreateSampler(const RHISamplerDesc& desc) {
	VkSamplerCreateInfo info{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, nullptr, 0 };
	info.minFilter = ToFilter(desc.Filter);
	info.magFilter = info.minFilter;
	info.addressModeU = ToAddressMode(desc.AddressU);
	info.addressModeV = ToAddressMode(desc.AddressV);
	info.addressModeW = ToAddressMode(desc.AddressW);
	info.mipmapMode = desc.Filter == ESamplerFilter::Trilinear ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
	info.mipLodBias = desc.LODBias;
	info.minLod = desc.MinLOD;
	info.maxLod = desc.MaxLOD;
	if(desc.Filter == ESamplerFilter::AnisotropicPoint || desc.Filter == ESamplerFilter::AnisotropicLinear) {
		info.maxAnisotropy = Math::FMin(desc.MaxAnisotropy, m_Device->GetProperties().limits.maxSamplerAnisotropy);
	}
	info.anisotropyEnable = static_cast<VkBool32>(info.maxAnisotropy > 1.0f);
	info.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	VkSampler samplerHandle;
	if (VK_SUCCESS != vkCreateSampler(m_Device->GetDevice(), &info, nullptr, &samplerHandle)) {
		return nullptr;
	}
	return new VulkanRHISampler(desc, GetDevice(), samplerHandle);
}

RHIFencePtr VulkanRHI::CreateFence(bool sig) {
	VkFenceCreateInfo info;
	info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	info.pNext = nullptr;
	info.flags = sig ? VK_FENCE_CREATE_SIGNALED_BIT : 0; // the fence is initialized as signaled
	VkFence fence;
	if(VK_SUCCESS == vkCreateFence(m_Device->GetDevice(), &info, nullptr, &fence)) {
		return new VulkanRHIFence(fence, GetDevice());
	}
	return nullptr;
}

RHIShaderPtr VulkanRHI::CreateShader(EShaderStageFlagBit type, const char* codeData, size_t codeSize, const char* entryFunc) {
	return new VulkanRHIShader(type, codeData, codeSize, entryFunc, GetDevice());
}

RHIGraphicsPipelineStatePtr VulkanRHI::CreateGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc) {
	return new VulkanRHIGraphicsPipelineState(desc, GetDevice()) ;
}


RHIComputePipelineStatePtr VulkanRHI::CreateComputePipelineState(const RHIComputePipelineStateDesc& desc) {
	return new VulkanRHIComputePipelineState(desc, GetDevice());
}

RHIRenderPassPtr VulkanRHI::CreateRenderPass(const RHIRenderPassDesc& desc) {
	return new VulkanRHIRenderPass(desc, GetDevice());
}

RHIShaderParameterSetPtr VulkanRHI::CreateShaderParameterSet(const RHIShaderParemeterLayout& layout) {
	DescriptorSetHandle handle = m_Device->GetDescriptorMgr()->AllocateDS(layout);
	if(handle.IsValid()) {
		return new VulkanRHIShaderParameterSet(handle, m_Device.Get());
	}
	return nullptr;
}

RHICommandBufferPtr VulkanRHI::CreateCommandBuffer() {
	return m_Device->GetCommandMgr()->NewCmd();
}

void VulkanRHI::SubmitCommandBuffer(RHICommandBuffer* cmd, RHIFence* fence) {
	VkCommandBuffer cmdHandle = dynamic_cast<VulkanRHICommandBuffer*>(cmd)->GetHandle();
	VkFence fenceHandle = dynamic_cast<VulkanRHIFence*>(fence)->GetFence();
	m_Device->GetCommandMgr()->AddGraphicsSubmit({&cmdHandle, 1}, fenceHandle);
}

void VulkanRHI::SubmitCommandBuffer(TArrayView<RHICommandBuffer*> cmds, RHIFence* fence) {
	TFixedArray<VkCommandBuffer> vkHandles(cmds.Size());
	VulkanRHIFence* vkFence = dynamic_cast<VulkanRHIFence*>(fence);
	for(uint32 i=0; i<cmds.Size(); ++i) {
		vkHandles[i] = dynamic_cast<VulkanRHICommandBuffer*>(cmds[i])->GetHandle();
	}
	m_Device->GetCommandMgr()->AddGraphicsSubmit({ vkHandles.Data(), cmds.Size() }, vkFence->GetFence());
}

void VulkanRHI::Present() {
	VkSemaphore imageAvailableSmp = m_Swapchain->AcquireImage();
	if(VK_NULL_HANDLE == imageAvailableSmp) {
		LOG_DEBUG("VulkanRHI::Present Could not acquire a image!");
		return;
	}
	VkSemaphore completeCmdSmp = m_Device->GetCommandMgr()->Submit(imageAvailableSmp);
	if(!m_Swapchain->Present(completeCmdSmp)) {
		LOG_DEBUG("VUlkanRHI::Present Could not present!");
	}
}

VkPipelineLayout VulkanRHI::CreatePipelineLayout(const RHIPipelineLayout& rhiLayout) {
	uint32 layoutCount = rhiLayout.Size();
	TFixedArray<VkDescriptorSetLayout> layouts(layoutCount);
	for (uint32 i = 0; i < layoutCount; ++i) {
		layouts[i] = m_Device->GetDescriptorMgr()->GetLayoutHandle(rhiLayout[i]);
	}
	VkPipelineLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0 };
	layoutInfo.setLayoutCount = layoutCount;
	layoutInfo.pSetLayouts = layouts.Data();
	VkPipelineLayout layoutHandle;
	if(VK_SUCCESS == vkCreatePipelineLayout(m_Device->GetDevice(), &layoutInfo, nullptr, &layoutHandle)) {
		return layoutHandle;
	}
	return VK_NULL_HANDLE;
}