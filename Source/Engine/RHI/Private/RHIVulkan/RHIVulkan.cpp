#include "RHIVulkan.h"
#include "VulkanConverter.h"
#include "VulkanBuilder.h"
#include "Math/Public/MathBase.h"
#include "VulkanDescriptorSet.h"

#define RETURN_RHI_PTR(cls, hd)\
cls##Vk* cls##Ptr = new cls##Vk;\
cls##Ptr->handle = hd; \
return reinterpret_cast<cls*>(cls##Ptr)

RHIVulkan::RHIVulkan(const RHIInitDesc& desc) {
	ERHIFormat swapchainFormat = ERHIFormat::B8G8R8A8_SRGB;

	// initialize context
	ContextBuilder initializer(m_Context);
	initializer.Desc.AppName = desc.AppName;
	initializer.Desc.WindowHandle = desc.WindowHandle;
	initializer.Desc.IntegratedGPU = desc.IntegratedGPU;
	initializer.Desc.EnableDebug = desc.EnableDebug;
	initializer.Desc.SurfaceFormat = { ToVkFormat(swapchainFormat), VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	initializer.Build();

	VulkanSwapchain::Initialize(&m_Context);
	VulkanMemMgr::Initialize(&m_Context);
	VulkanCommandMgr::Initialize(&m_Context);
	VulkanDSMgr::Initialize(m_Context.Device);
	PRINT("RHI: Vulkan initialized successfully!");
}

RHIVulkan::~RHIVulkan() {
	//vma
	VulkanSwapchain::Release();
	VulkanMemMgr::Release();
	VulkanCommandMgr::Release();
	VulkanDSMgr::Release();
	ContextBuilder initializer(m_Context);
	initializer.Release();
}

ERHIFormat RHIVulkan::GetDepthFormat() {
	switch (m_Context.DepthFormat) {
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

USize2D RHIVulkan::GetRenderArea() {
	return VulkanSwapchain::Instance()->GetSize();
}

#pragma endregion

VkDevice RHIVulkan::GetDevice() {
	return InstanceVulkan()->m_Context.Device;
}

RHIVulkan* RHIVulkan::InstanceVulkan() {
	return dynamic_cast<RHIVulkan*>(Instance());
}

const VulkanContext& RHIVulkan::GetContext() {
	return m_Context;
}

RHIBuffer* RHIVulkan::CreateBuffer(const RHIBufferDesc& desc, void* defaultData) {
	VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, nullptr, 0 };
	bufferInfo.usage = ToBufferUsage(desc.Flags);
	bufferInfo.size = desc.ByteSize;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VkBuffer buffer;
	if(VK_SUCCESS != vkCreateBuffer(GetDevice(), &bufferInfo, nullptr, &buffer)) {
		return nullptr;
	}
	VulkanMem mem;
	VkMemoryPropertyFlags memoryProperty = ToBufferMemoryProperty(desc.Flags);
	if (!VulkanMemMgr::Instance()->BindBuffer(mem, buffer, EMemoryType::GPU, memoryProperty)) {
		vkDestroyBuffer(GetDevice(), buffer, nullptr);
		return nullptr;
	}
	return new RHIVulkanBufferWithMem(desc, buffer, std::move(mem));
}

RHITexture* RHIVulkan::CreateTexture(const RHITextureDesc& desc, void* defaultData) {
	VkImageCreateInfo imageInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, nullptr, 0 };
	imageInfo.imageType = ToImageType(desc.Dimension);
	imageInfo.format = ToVkFormat(desc.Format);
	imageInfo.extent = { desc.Size.w, desc.Size.h, desc.Size.h };
	imageInfo.mipLevels = desc.NumMips;
	imageInfo.arrayLayers = desc.ArraySize;
	imageInfo.samples = (VkSampleCountFlagBits)desc.Samples;
	imageInfo.usage = ToImageUsage(desc.Flags);
	VkImage imageHandle;
	if(VK_SUCCESS != vkCreateImage(GetDevice(), &imageInfo, nullptr, &imageHandle)) {
		return nullptr;
	}

	// memory
	VulkanMem mem;
	VkMemoryPropertyFlags memoryProperty = ToImageMemoryProperty(desc.Flags);
	if(!VulkanMemMgr::Instance()->BindImage(mem, imageHandle, EMemoryType::GPU, memoryProperty)) {
		vkDestroyImage(GetDevice(), imageHandle, nullptr);
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
	vkCreateImageView(GetDevice(), &imageViewCreateInfo, nullptr, &viewHandle);

	return new RHIVulkanTextureWithMem(desc, imageHandle, viewHandle, std::move(mem));
}

RHISampler* RHIVulkan::CreateSampler(const RHISamplerDesc& desc) {
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
		info.maxAnisotropy = Math::FMin(desc.MaxAnisotropy, m_Context.DeviceProperties.limits.maxSamplerAnisotropy);
	}
	info.anisotropyEnable = static_cast<VkBool32>(info.maxAnisotropy > 1.0f);
	info.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	VkSampler samplerHandle;
	if (VK_SUCCESS != vkCreateSampler(GetDevice(), &info, nullptr, &samplerHandle)) {
		return nullptr;
	}
	return new RHIVulkanSampler(desc, samplerHandle);
}

RHIFence* RHIVulkan::CreateFence(bool sig) {
	VkFenceCreateInfo info;
	info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	info.pNext = nullptr;
	info.flags = sig ? VK_FENCE_CREATE_SIGNALED_BIT : 0; // the fence is initialized as signaled
	VkFence fence;
	if(VK_SUCCESS == vkCreateFence(GetDevice(), &info, nullptr, &fence)) {
		return new RHIVulkanFence(fence);
	}
	return nullptr;
}

RHIShader* RHIVulkan::CreateShader(EShaderStageFlagBit type, const char* codeData, size_t codeSize, const char* entryFunc) {
	return new RHIVulkanShader(type, codeData, codeSize, entryFunc);
}

RHIGraphicsPipelineState* RHIVulkan::CreateGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc) {
	VkPipelineLayout layout = CreatePipelineLayout(desc.Layout);
	return layout ? new RHIVulkanGraphicsPipelineState(desc, layout) : nullptr;
}


RHIComputePipelineState* RHIVulkan::CreateComputePipelineState(const RHIComputePipelineStateDesc& desc) {
	VkPipelineLayout layout = CreatePipelineLayout(desc.Layout);
	return layout ? new RHIVulkanComputePipelineState(desc, layout) : nullptr;
}

RHIRenderPass* RHIVulkan::CreateRenderPass(const RHIRenderPassDesc& desc) {
	return new RHIVulkanRenderPass(desc);
}

RHIShaderParameterSet* RHIVulkan::CreateShaderParameterSet(const RHIShaderParemeterLayout& layout) {
	DescriptorSetHandle handle = VulkanDSMgr::Instance()->AllocateDS(layout);
	if(handle.IsValid()) {
		return new RHIVulkanShaderParameterSet(handle);
	}
	return nullptr;
}

RHICommandBuffer* RHIVulkan::CreateCommandBuffer() {
	VkCommandBuffer handle = VulkanCommandMgr::Instance()->NewCmd();
	if(VK_NULL_HANDLE == handle) {
		return nullptr;
	}
	return new RHIVulkanCommandBuffer(handle);
}

void RHIVulkan::SubmitCommandBuffer(RHICommandBuffer* cmd, RHIFence* fence) {
	VkCommandBuffer cmdHandle = dynamic_cast<RHIVulkanCommandBuffer*>(cmd)->GetHandle();
	VkFence fenceHandle = dynamic_cast<RHIVulkanFence*>(fence)->GetFence();
	VulkanCommandMgr::Instance()->AddGraphicsSubmit({&cmdHandle, 1}, fenceHandle);
}

void RHIVulkan::SubmitCommandBuffer(TArrayView<RHICommandBuffer*> cmds, RHIFence* fence) {
	TFixedArray<VkCommandBuffer> vkHandles(cmds.Size());
	RHIVulkanFence* vkFence = dynamic_cast<RHIVulkanFence*>(fence);
	for(uint32 i=0; i<cmds.Size(); ++i) {
		vkHandles[i] = dynamic_cast<RHIVulkanCommandBuffer*>(cmds[i])->GetHandle();
	}
	VulkanCommandMgr::Instance()->AddGraphicsSubmit({ vkHandles.Data(), cmds.Size() }, vkFence->GetFence());
}

void RHIVulkan::Present() {
	VulkanDSMgr::Instance()->Update();
	VkSemaphore imageAvailableSmp = VulkanSwapchain::Instance()->AcquireImage();
	if(VK_NULL_HANDLE == imageAvailableSmp) {
		PRINT_DEBUG("RHIVulkan::Present Could not acquire a image!");
		return;
	}
	VkSemaphore completeCmdSmp = VulkanCommandMgr::Instance()->Submit(imageAvailableSmp);
	if(!VulkanSwapchain::Instance()->Present(completeCmdSmp)) {
		PRINT_DEBUG("RHIVulkan::Present Could not present!");
	}
}

VkPipelineLayout RHIVulkan::CreatePipelineLayout(const RHIPipelineLayout& rhiLayout) {
	uint32 layoutCount = rhiLayout.Size();
	TFixedArray<VkDescriptorSetLayout> layouts(layoutCount);
	for (uint32 i = 0; i < layoutCount; ++i) {
		layouts[i] = VulkanDSMgr::Instance()->GetLayoutHandle(rhiLayout[i]);
	}
	VkPipelineLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0 };
	layoutInfo.setLayoutCount = layoutCount;
	layoutInfo.pSetLayouts = layouts.Data();
	VkPipelineLayout layoutHandle;
	if(VK_SUCCESS == vkCreatePipelineLayout(m_Context.Device, &layoutInfo, nullptr, &layoutHandle)) {
		return layoutHandle;
	}
	return VK_NULL_HANDLE;
}
