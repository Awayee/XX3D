#include "RHIVulkan.h"
#include "VulkanConverter.h"
#include "VulkanBuilder.h"
#include "Math/Public/MathBase.h"
#include "VulkanDescriptorSet.h"

#define RETURN_RHI_PTR(cls, hd)\
cls##Vk* cls##Ptr = new cls##Vk;\
cls##Ptr->handle = hd; \
return reinterpret_cast<cls*>(cls##Ptr)



void RHIVulkan::CreateCommandPools() {
	// graphics command pool
	{
		VkCommandPoolCreateInfo commandPoolCreateInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr};
		commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		commandPoolCreateInfo.queueFamilyIndex = m_Context.GraphicsQueue.FamilyIndex;
		VK_CHECK(vkCreateCommandPool(GetDevice(), &commandPoolCreateInfo, nullptr, &m_RHICommandPool), "VkCreateCommandPool");
	}
}

RHIVulkan::RHIVulkan(const RHIInitDesc& desc) {
	// initialize context
	ContextBuilder initializer(m_Context);
	initializer.AppName = desc.AppName;
	initializer.WindowHandle = desc.WindowHandle;
	if (desc.IntegratedGPU){
		initializer.Flags |= ContextBuilder::INTEGRATED_GPU;
	}
	if (desc.EnableDebug) {
		initializer.Flags |= ContextBuilder::ENABLE_DEBUG;
	}
	initializer.Build();

	// create swap chain
	m_SwapChain.reset(new VulkanSwapchain(&m_Context));

	// create memory manager
	m_MemMgr.reset(new VulkanMemMgr(&m_Context));

	// create command manager
	m_CmdMgr.reset(new VulkanCommandMgr(&m_Context));

	// create descriptor set manager
	m_DSMgr.reset(new VulkanDSMgr(m_Context.Device));

	PRINT("RHI: Vulkan initialized successfully!");
}

RHIVulkan::~RHIVulkan() {
	//vma
	m_MemMgr.reset();
	m_SwapChain.reset();
	m_CmdMgr.reset();
	m_DSMgr.reset();
	ContextBuilder initializer(m_Context);
	initializer.Release();
}

RHISwapChain* RHIVulkan::GetSwapChain() {
	return static_cast<RHISwapChain*>(m_SwapChain.get());
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

#pragma endregion

VkDevice RHIVulkan::GetDevice() {
	return InstanceVulkan()->m_Context.Device;
}

RHIVulkan* RHIVulkan::InstanceVulkan() {
	return dynamic_cast<RHIVulkan*>(Instance());
}

RHIBuffer* RHIVulkan::CreateBuffer(const RHIBufferDesc& desc) {
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
	if (!m_MemMgr->BindBuffer(mem, buffer, EMemoryType::GPU, memoryProperty)) {
		vkDestroyBuffer(GetDevice(), buffer, nullptr);
		return nullptr;
	}
	return new RHIVkBuffer(desc, buffer, std::move(mem));
}

RHITexture* RHIVulkan::CreateTexture(const RHITextureDesc& desc) {
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
	if(!m_MemMgr->BindImage(mem, imageHandle, EMemoryType::GPU, memoryProperty)) {
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

	return new RHIVkTextureWithMem(desc, imageHandle, viewHandle, std::move(mem));
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
	return new RHIVkSampler(desc, samplerHandle);
}

RHIFence* RHIVulkan::CreateFence(bool sig) {
	VkFenceCreateInfo info;
	info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	info.pNext = nullptr;
	info.flags = sig ? VK_FENCE_CREATE_SIGNALED_BIT : 0; // the fence is initialized as signaled
	VkFence fence;
	if(VK_SUCCESS == vkCreateFence(GetDevice(), &info, nullptr, &fence)) {
		return new RHIVkFence(fence);
	}
	return nullptr;
}

RHIShader* RHIVulkan::CreateShader(EShaderStageFlagBit type, const char* codeData, size_t codeSize, const char* entryFunc) {
	return new RHIVulkanShader(type, codeData, codeSize, entryFunc);
}

RHIGraphicsPipelineState* RHIVulkan::CreateGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc) {
	return new RHIVulkanGraphicsPipelineState(desc);
}

RHIComputePipelineState* RHIVulkan::CreateComputePipelineState(const RHIComputePipelineStateDesc& desc) {
	return new RHIVulkanComputePipelineState(desc);
}

RHIRenderPass* RHIVulkan::CreateRenderPass(const RHIRenderPassDesc& desc) {
	return new RHIVulkanRenderPass(desc);
}

RHIShaderParameterSet* RHIVulkan::CreateShaderParameterSet(const RHIShaderParemeterLayout& layout) {
	DescriptorSetHandle handle = m_DSMgr->AllocateDS(layout);
	if(handle.IsValid()) {
		return new RHIVulkanShaderParameterSet(m_DSMgr.get(), handle);
	}
	return nullptr;
}

RHICommandBuffer* RHIVulkan::CreateCommandBuffer() {
	VkCommandBuffer handle = m_CmdMgr->NewCmd();
	if(VK_NULL_HANDLE == handle) {
		return nullptr;
	}
	return new RHIVulkanCommandBuffer(handle, m_CmdMgr.get());
}

void RHIVulkan::SubmitCommandBuffer(TArrayView<RHICommandBuffer*> cmds, RHIFence* fence) {
	TempArray<VkCommandBuffer> vkHandles(cmds.Size());
	RHIVkFence* vkFence = dynamic_cast<RHIVkFence*>(fence);
	for(uint32 i=0; i<cmds.Size(); ++i) {
		vkHandles[i] = dynamic_cast<RHIVulkanCommandBuffer*>(cmds[i])->GetHandle();
	}
	m_CmdMgr->AddGraphicsSubmit(cmds.Size(), vkHandles.Data(), vkFence->GetFence());
}

void RHIVulkan::Present() {
	m_DSMgr->Update();

	TVector<VkSemaphore> smps;
	m_CmdMgr->GetLastSignalSmps(smps);
	m_CmdMgr->Submit();
	m_SwapChain->Present({ smps });
}
