#include "RHIVulkan.h"
#include "VulkanUtil.h"
#include "VulkanConverter.h"
#include "VulkanBuilder.h"
#include "Math/Public/MathBase.h"
#include "VulkanDesc.h"

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
	m_SwapChain.reset(new RHIVulkanSwapChain(&m_Context));

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
	vkDestroyCommandPool(GetDevice(), m_RHICommandPool, nullptr);
	VulkanDSMgr::Release();
	ContextBuilder initializer(m_Context);
	initializer.Release();
}

RSVkImGuiInitInfo RHIVulkan::GetImGuiInitInfo() {
	RSVkImGuiInitInfo info;
	info.windowHandle = m_Context.WindowHandle;
	info.instance = m_Context.Instance;
	info.device = m_Context.Device;
	info.physicalDevice = m_Context.PhysicalDevice;
	info.queueIndex = m_Context.GraphicsQueue.FamilyIndex;
	info.queue = m_Context.GraphicsQueue.Handle;
	info.descriptorPool = m_DescriptorPool;
	return info;
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

RRenderPass* RHIVulkan::CreateRenderPass(TConstArrayView<RSAttachment> attachments, TConstArrayView<RSubPassInfo> subpasses, TConstArrayView<RSubpassDependency> dependencies) {
	uint32 i;
	TempArray<VkAttachmentDescription> attachmentsVk(attachments.Size());
	for(i=0; i<attachments.Size(); ++i) {
		attachmentsVk[i] = ResolveAttachmentDesc(attachments[i]);
	}

	TempArray<VkSubpassDescription> subpassesVk(subpasses.Size());
	TempArray<TVector<VkAttachmentReference>> inputAttachments(subpasses.Size());
	TempArray<TVector<VkAttachmentReference>> colorAttachments(subpasses.Size());
	TempArray<VkAttachmentReference> depthAttachments(subpasses.Size());

	for(i=0; i< subpasses.Size(); ++i) {
		subpassesVk[i].flags = 0;
		subpassesVk[i].pipelineBindPoint = (VkPipelineBindPoint)subpasses[i].Type;
		uint32 j;
		for (j = 0; j < subpasses[i].InputAttachments.Size(); ++j) {
			inputAttachments[i].PushBack({ subpasses[i].InputAttachments[j].Index, ToVkImageLayout(subpasses[i].InputAttachments[j].Layout)});
		}
		subpassesVk[i].inputAttachmentCount = inputAttachments[i].Size();
		subpassesVk[i].pInputAttachments = inputAttachments[i].Data();
		for (j = 0; j < subpasses[i].ColorAttachments.Size(); ++j) {
			colorAttachments[i].PushBack({ subpasses[i].ColorAttachments[j].Index, ToVkImageLayout(subpasses[i].ColorAttachments[j].Layout)});
		}
		subpassesVk[i].colorAttachmentCount = colorAttachments[i].Size();
		subpassesVk[i].pColorAttachments = colorAttachments[i].Data();
		if(IMAGE_LAYOUT_UNDEFINED != subpasses[i].DepthStencilAttachment.Layout) {
			depthAttachments[i] = { subpasses[i].DepthStencilAttachment.Index, ToVkImageLayout(subpasses[i].DepthStencilAttachment.Layout)};
			subpassesVk[i].pDepthStencilAttachment = &depthAttachments[i];
		}
		else {
			subpassesVk[i].pDepthStencilAttachment = VK_NULL_HANDLE;
		}
		// empty resolve
		subpassesVk[i].preserveAttachmentCount = 0;
		subpassesVk[i].pPreserveAttachments = nullptr;
		subpassesVk[i].pResolveAttachments = nullptr;
	}

	TempArray<VkSubpassDependency> dependenciesVk(dependencies.Size());
	for(i=0; i<dependencies.Size();++i) {
		dependenciesVk[i] = ResolveSubpassDependency(dependencies[i]);
	}

	VkRenderPassCreateInfo info{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, nullptr };
	info.attachmentCount = attachments.Size();
	info.pAttachments = attachmentsVk.Data();
	info.subpassCount = subpasses.Size();
	info.pSubpasses= subpassesVk.Data();
	info.dependencyCount = dependencies.Size();
	info.pDependencies = dependenciesVk.Data();
	VkRenderPass handle;
	if(VK_SUCCESS!=vkCreateRenderPass(GetDevice(), &info, nullptr, &handle)){
		return nullptr;
	}
	RRenderPassVk* pass = new RRenderPassVk;
	pass->handle = handle;
	pass->m_Clears.Resize(attachments.Size());
	for(i=0; i< pass->m_Clears.Size(); ++i) {
		pass->m_Clears[i] = ResolveClearValue(attachments[i].Clear);
	}
	return pass;
}

RRenderPass* RHIVulkan::CreateRenderPass(TConstArrayView<RSAttachment> colorAttachments, const RSAttachment* depthAttachment) {
	uint32 i;
	uint32 attachmentCount = colorAttachments.Size() + (nullptr != depthAttachment);
	TempArray<VkAttachmentDescription> attachmentsVk(attachmentCount);
	TempArray<VkAttachmentReference> colorAttachmentRef(colorAttachments.Size());
	for (i = 0; i < colorAttachments.Size(); ++i) {
		attachmentsVk[i] = ResolveAttachmentDesc(colorAttachments[i]);
		colorAttachmentRef[i] = { i, ToVkImageLayout(colorAttachments[i].FinalLayout) };
	}
	VkAttachmentReference depthAttachmentRef;
	if(nullptr != depthAttachment) {
		depthAttachmentRef.attachment = colorAttachments.Size();
		depthAttachmentRef.layout = ToVkImageLayout(depthAttachment->FinalLayout);
		attachmentsVk[colorAttachments.Size()] = ResolveAttachmentDesc(*depthAttachment);
	}
	VkSubpassDescription subpass;
	subpass.flags = 0;
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = nullptr;
	subpass.colorAttachmentCount = colorAttachments.Size();
	subpass.pColorAttachments = colorAttachmentRef.Data();
	subpass.pResolveAttachments = nullptr;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = nullptr;
	subpass.pDepthStencilAttachment = (nullptr != depthAttachment) ? &depthAttachmentRef : nullptr;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo info{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, nullptr };
	info.attachmentCount = attachmentCount;
	info.pAttachments = attachmentsVk.Data();
	info.subpassCount = 1;
	info.pSubpasses = &subpass;
	info.dependencyCount = 1;
	info.pDependencies = &dependency;

	VkRenderPass handle;
	if (VK_SUCCESS != vkCreateRenderPass(GetDevice(), &info, nullptr, &handle)) {
		return nullptr;
	}
	RRenderPassVk* pass = new RRenderPassVk;
	pass->handle = handle;
	return static_cast<RRenderPass*>(pass);

}

void RHIVulkan::DestroyRenderPass(RRenderPass* pass) {
	RRenderPassVk* vkPass = static_cast<RRenderPassVk*>(pass);
	vkDestroyRenderPass(GetDevice(), vkPass->handle, nullptr);
	delete vkPass;
}
RDescriptorSetLayout* RHIVulkan::CreateDescriptorSetLayout(TConstArrayView<RSDescriptorSetLayoutBinding> bindings) {
	VkDescriptorSetLayoutCreateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	info.pNext = nullptr;
	info.flags = 0;
	info.bindingCount = bindings.Size();
	TempArray<VkDescriptorSetLayoutBinding> bindingsVk(bindings.Size());
	for(uint32 i=0; i< bindings.Size(); ++i) {
		bindingsVk[i].binding = i;
		bindingsVk[i].descriptorType = (VkDescriptorType)bindings[i].descriptorType;
		bindingsVk[i].descriptorCount = bindings[i].descriptorCount;
		bindingsVk[i].stageFlags = (VkShaderStageFlags)bindings[i].stageFlags;
		bindingsVk[i].pImmutableSamplers = nullptr;
	}
	info.pBindings = bindingsVk.Data();
	VkDescriptorSetLayout handle;
	if(VK_SUCCESS != vkCreateDescriptorSetLayout(GetDevice(), &info, nullptr, &handle)) {
		return nullptr;
	}
	RDescriptorSetLayoutVk* descriptorSetLayout = new RDescriptorSetLayoutVk;
	descriptorSetLayout->handle = handle;
	return descriptorSetLayout;

}

void RHIVulkan::DestroyDescriptorSetLayout(RDescriptorSetLayout* descriptorSetLayout) {
	vkDestroyDescriptorSetLayout(GetDevice(), ((RDescriptorSetLayoutVk*)descriptorSetLayout)->handle, nullptr);
}

RDescriptorSet* RHIVulkan::AllocateDescriptorSet(const RDescriptorSetLayout* layout) {
	VkDescriptorSetAllocateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	info.pNext = nullptr;
	info.descriptorPool = m_DescriptorPool;
	info.descriptorSetCount = 1;
	info.pSetLayouts = &((RDescriptorSetLayoutVk*)layout)->handle;
	VkDescriptorSet handle;
	if (VK_SUCCESS != vkAllocateDescriptorSets(GetDevice(), &info, &handle)) {
		return nullptr;
	}
	RDescriptorSetVk* descriptorSet = new RDescriptorSetVk;
	descriptorSet->handle = handle;
	return descriptorSet;
}

void RHIVulkan::FreeDescriptorSet(RDescriptorSet* descriptorSet) {
	RDescriptorSetVk* descirptorSetVk = (RDescriptorSetVk*)descriptorSet;
	vkFreeDescriptorSets(GetDevice(), m_DescriptorPool, 1, &descirptorSetVk->handle);
	delete descirptorSetVk;
}

void RHIVulkan::ImmediateSubmit(const CommandBufferFunc& func) {
	// allocate
	VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocateInfo.pNext = nullptr;
	allocateInfo.commandPool = m_RHICommandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = 1;
	VkCommandBuffer cmd;
	VK_CHECK(vkAllocateCommandBuffers(GetDevice(), &allocateInfo, &cmd), "Failed to allocate command buffers!");
	RHIVulkanCommandBuffer rhiCmd(cmd, allocateInfo.commandPool);

	// begin
	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.pNext = nullptr;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	beginInfo.pInheritanceInfo = nullptr;
	VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo), "Failed to begin command buffer!");

	// run
	func(&rhiCmd);

	// end
	vkEndCommandBuffer(cmd);

	// submit
	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;
	vkQueueSubmit(m_Context.GraphicsQueue.Handle, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_Context.GraphicsQueue.Handle);
}

void RHIVulkan::SubmitCommandBuffer(const RHICommandBuffer* cmd, RHIFence* fence, RHISwapChain* swapChain) {
}

int RHIVulkan::QueueSubmitPresent(RHICommandBuffer* cmd, uint8 frameIndex, RHIFence* fence) {
	// submit command buffer
	//VkSemaphore semaphores[2] = { m_ImageAvaliableForTextureCopySemaphores[m_CurrentFrame], m_PresentationFinishSemaphores[m_CurrentFrame] };
	VkSemaphore semaphores[1] = { m_PresentationFinishSemaphores[frameIndex] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	VkCommandBuffer vkCmd = dynamic_cast<RHIVulkanCommandBuffer*>(cmd)->m_VkCmd;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &m_ImageAvaliableSemaphores[frameIndex];
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &vkCmd;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = semaphores;
	const RHIVkFence* fenceVk = dynamic_cast<RHIVkFence*>(fence);
	VK_CHECK(vkQueueSubmit(m_Context.GraphicsQueue.Handle, 1, &submitInfo, fenceVk->m_Handle), "Failed to submit graphics queue!");

	// present
	m_SwapChain->Present();
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
	RHIVkBuffer* rhiBuffer = new RHIVkBuffer(desc);
	rhiBuffer->m_Buffer = buffer;
	rhiBuffer->m_Mem = std::move(mem);
	return rhiBuffer;
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

	RHIVkTextureWithMem* textureVk = new RHIVkTextureWithMem(desc);
	textureVk->m_Image = imageHandle;
	textureVk->m_View = viewHandle;
	textureVk->m_Mem = mem;
	return textureVk;
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
	RHIVkSampler* sampler = new RHIVkSampler(desc);
	sampler->m_Sampler = samplerHandle;
	return sampler;
}

RHIFence* RHIVulkan::CreateFence(bool sig) {
	VkFenceCreateInfo info;
	info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	info.pNext = nullptr;
	info.flags = sig ? VK_FENCE_CREATE_SIGNALED_BIT : 0; // the fence is initialized as signaled
	RHIVkFence* fenceVk = new RHIVkFence;
	vkCreateFence(GetDevice(), &info, nullptr, &fenceVk->m_Handle);
	return static_cast<RHIFence*>(fenceVk);
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

RHICommandBuffer* RHIVulkan::CreateCommandBuffer() {
	VkCommandBuffer handle = m_CmdMgr->NewCmd();
	if(VK_NULL_HANDLE == handle) {
		return nullptr;
	}
	return new RHIVulkanCommandBuffer(handle, m_CmdMgr.get());
}
