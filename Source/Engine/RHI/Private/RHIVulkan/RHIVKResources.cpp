#include "RHIVKResources.h"
#include "VulkanUtil.h"
#include "RHIVulkan.h"
#include "VulkanBuilder.h"
#include "VulkanDesc.h"
#define VK_SET_OBJECT_NAME(type, handle, name) do{\
VkDebugUtilsObjectNameInfoEXT info{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, nullptr, type, reinterpret_cast<uint64>(handle), name};\
vkSetDebugUtilsObjectNameEXT(RHIVulkan::GetDevice(), &info);\
}while(0)

RHIVulkanSwapChain::RHIVulkanSwapChain(const VulkanContext* context) : m_ContextPtr(context) {
	ASSERT(m_ContextPtr, "");
	CreateSwapChain();

	// Create semaphores
	m_FrameRes.Resize(MAX_FRAME_COUNT);
	VkSemaphoreCreateInfo smpInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr };
	for (FrameResource& res : m_FrameRes) {
		vkCreateSemaphore(m_ContextPtr->Device, &smpInfo, nullptr, &res.ImageAvailableSmp);
		vkCreateSemaphore(m_ContextPtr->Device, &smpInfo, nullptr, &res.PreparePresentSmp);
	}
}

RHIVulkanSwapChain::~RHIVulkanSwapChain() {
	ClearSwapChain();
	for (FrameResource& res : m_FrameRes) {
		vkDestroySemaphore(m_ContextPtr->Device, res.ImageAvailableSmp, nullptr);
		vkDestroySemaphore(m_ContextPtr->Device, res.PreparePresentSmp, nullptr);
	}
}

bool RHIVulkanSwapChain::Present() {
	if (m_Prepared) {
		VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, nullptr };
		FrameResource& lastFrame = m_FrameRes[m_CurFrame];
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &lastFrame.PreparePresentSmp;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_Handle;
		presentInfo.pImageIndices = &m_FrameRes[m_CurFrame].ImageIdx;
		vkQueuePresentKHR(m_ContextPtr->PresentQueue.Handle, &presentInfo);
		m_CurFrame = (m_CurFrame + 1) % m_FrameRes.Size();
		m_Prepared = false;
	}
	FrameResource& currentFrame = m_FrameRes[m_CurFrame];
	VkResult res = vkAcquireNextImageKHR(m_ContextPtr->Device, m_Handle, WAIT_MAX, currentFrame.ImageAvailableSmp, VK_NULL_HANDLE, &currentFrame.ImageIdx);
	if (VK_SUCCESS == res) {
		m_Prepared = true;
		return true;
	}
	return false;
}

RHIVkBuffer::~RHIVkBuffer() {
	VkDevice device = RHIVulkan::GetDevice();
	vkDestroyBuffer(device, m_Buffer, nullptr);
	m_Mem.Free();
}

void RHIVkBuffer::SetName(const char* name) {
	VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_BUFFER, m_Buffer, name);
}

void RHIVkBuffer::UpdateData(const void* data, size_t byteSize) {
	void* mappedData = m_Mem.Map();
	memcpy(mappedData, data, byteSize);
	m_Mem.Unmap();
}

RHIVkTexture::~RHIVkTexture() {
	VkDevice device = RHIVulkan::GetDevice();
	vkDestroyImage(device, m_Image, nullptr);
	vkDestroyImageView(device, m_View, nullptr);
}

void RHIVkTexture::SetName(const char* name) {
	VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_IMAGE, m_Image, name);
	VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_IMAGE_VIEW, m_View, name);
}

RHIVkTextureWithMem::~RHIVkTextureWithMem() {
	m_Mem.Free();
}

RHIVkSampler::~RHIVkSampler() {
	VkDevice device = RHIVulkan::GetDevice();
	vkDestroySampler(device, m_Sampler, nullptr);
}

void RHIVkSampler::SetName(const char* name) {
	VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_SAMPLER, m_Sampler, name);
}


RHIVkFence::~RHIVkFence() {
	VkDevice device = RHIVulkan::GetDevice();
	vkDestroyFence(device, m_Handle, nullptr);
}

void RHIVkFence::Wait() {
	vkWaitForFences(RHIVulkan::GetDevice(), 1, &m_Handle, 1, WAIT_MAX);
}

void RHIVkFence::Reset() {
	vkResetFences(RHIVulkan::GetDevice(), 1, &m_Handle);
}

void RHIVkFence::SetName(const char* name) {
	VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_FENCE, m_Handle, name);
}

void RHIVulkanSwapChain::Resize(USize2D size) {
	ClearSwapChain();
	CreateSwapChain();
}

USize2D RHIVulkanSwapChain::GetExtent() {
	return { m_Width, m_Height };
}

void RHIVulkanSwapChain::CreateSwapChain() {
	SwapChainBuilder initializer(m_Handle, m_Images, m_Views);
	initializer.Extent = { m_Width, m_Height };
	initializer.SetContext(*m_ContextPtr);
	initializer.Build();
}

void RHIVulkanSwapChain::ClearSwapChain() {
	VkDevice device = RHIVulkan::GetDevice();
	vkDestroySwapchainKHR(device, m_Handle, nullptr);
	for (VkImage image : m_Images) {
		vkDestroyImage(device, image, nullptr);
	}for (VkImageView view : m_Views) {
		vkDestroyImageView(device, view, nullptr);
	}
}

RHIVulkanShader::RHIVulkanShader(EShaderStageFlagBit type, const char* code, size_t codeSize, const char* funcName): RHIShader(type) {
	VkDevice device = RHIVulkan::GetDevice();
	VkShaderModuleCreateInfo shaderInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr, 0, codeSize, reinterpret_cast<const uint32*>(code)};
	vkCreateShaderModule(device, &shaderInfo, nullptr, &m_ShaderModule);
	m_EntryName = funcName;
}

RHIVulkanShader::~RHIVulkanShader() {
	VkDevice device = RHIVulkan::GetDevice();
	vkDestroyShaderModule(device, m_ShaderModule, nullptr);
}

void RHIVulkanShader::SetName(const char* name) {
	VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_SHADER_MODULE, m_ShaderModule, name);
}

VkPipelineShaderStageCreateInfo RHIVulkanShader::GetShaderStageInfo() const {
	VkPipelineShaderStageCreateInfo info{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0 };
	info.stage = ToVkShaderStageFlagBit(m_Type);
	info.module = m_ShaderModule;
	info.pName = m_EntryName.data();
	info.pSpecializationInfo = nullptr;
	return info;
}

inline VkPipelineLayout CreatePipelineLayout(const RHIPipelineLayout& rhiLayout) {
	VkDevice device = RHIVulkan::GetDevice();
	uint32 layoutCount = rhiLayout.Size();
	TempArray<VkDescriptorSetLayout> layouts(layoutCount);
	for (uint32 i = 0; i < layoutCount; ++i) {
		layouts[i] = VulkanDSMgr::Instance()->GetLayoutHandle(rhiLayout[i]);
	}
	VkPipelineLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0 };
	layoutInfo.setLayoutCount = layoutCount;
	layoutInfo.pSetLayouts = layouts.Data();
	VkPipelineLayout handle;
	vkCreatePipelineLayout(device, &layoutInfo, nullptr, &handle);
	return handle;
}

RHIVulkanGraphicsPipelineState::RHIVulkanGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc):RHIGraphicsPipelineState(desc) {
	m_PipelineLayout = CreatePipelineLayout(desc.Layout);
}

RHIVulkanGraphicsPipelineState::~RHIVulkanGraphicsPipelineState() {
	VkDevice device = RHIVulkan::GetDevice();
	vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
	vkDestroyPipeline(device, m_Pipeline, nullptr);
}

void RHIVulkanGraphicsPipelineState::SetName(const char* name) {
	m_Name = name;
}

VkPipeline RHIVulkanGraphicsPipelineState::GetPipelineHandle(VkRenderPass pass, uint32 subPass) {
	if (VK_NULL_HANDLE == m_Pipeline) {
		CreatePipelineHandle(pass, subPass);
	}
	return m_Pipeline;
}

void RHIVulkanGraphicsPipelineState::CreatePipelineHandle(VkRenderPass pass, uint32 subPass) {
	VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, nullptr, 0 };
	// shader stage

	auto& shaders = m_Desc.Shaders;
	uint32 shaderCount = shaders.Size();
	TempArray<VkPipelineShaderStageCreateInfo> shaderStages(shaderCount);
	for(uint32 i=0; i<shaderCount; ++i) {
		RHIVulkanShader* vkShader = dynamic_cast<RHIVulkanShader*>(shaders[i]);
		shaderStages[i] = vkShader->GetShaderStageInfo();
	}

	// vertex input
	auto& vertexInput = m_Desc.VertexInput;
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, nullptr, 0 };
	const uint32 attrCount = vertexInput.Attributes.Size();
	TempArray<VkVertexInputAttributeDescription> viAttrs(attrCount);
	for (uint32 i = 0; i < attrCount; ++i) {
		viAttrs[i].format = ToVkFormat(vertexInput.Attributes[i].Format);
		viAttrs[i].binding = vertexInput.Attributes[i].Binding;
		viAttrs[i].location = i;
		viAttrs[i].offset = vertexInput.Attributes[i].Offset;
	}
	vertexInputInfo.vertexAttributeDescriptionCount = attrCount;
	vertexInputInfo.pVertexAttributeDescriptions = viAttrs.Data();
	const uint32 bindingCount = vertexInput.Bindings.Size();
	TempArray<VkVertexInputBindingDescription> viBindings(bindingCount);
	for(uint32 i=0; i<bindingCount; ++i) {
		viBindings[i].binding = i;
		viBindings[i].inputRate = vertexInput.Bindings[i].PerInstance ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
		viBindings[i].stride = vertexInput.Bindings[i].Stride;
	}
	vertexInputInfo.vertexBindingDescriptionCount = bindingCount;
	vertexInputInfo.pVertexBindingDescriptions = viBindings.Data();

	// input assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, nullptr, 0 };
	inputAssemblyInfo.topology = ToPrimitiveTopology(m_Desc.PrimitiveTopology);
	inputAssemblyInfo.primitiveRestartEnable = false;

	// tessellation state TODO
	VkPipelineTessellationStateCreateInfo tessellationStateInfo{ VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO, nullptr, 0 };
	tessellationStateInfo.patchControlPoints = 0;

	// viewport state (dynamic)
	VkPipelineViewportStateCreateInfo viewportInfo{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, nullptr, 0 };
	viewportInfo.viewportCount = 1;
	viewportInfo.scissorCount = 1;

	// rasterization
	VkPipelineRasterizationStateCreateInfo rasterizationStateInfo = ToRasterizationStateCreateInfo(m_Desc.RasterizerState);

	// multi-sample
	VkPipelineMultisampleStateCreateInfo multiSampleInfo{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, nullptr, 0 };
	multiSampleInfo.rasterizationSamples = ToVkMultiSampleCount(m_Desc.NumSamples);

	// depth stencil
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo = ToDepthStencilStateCreateInfo(m_Desc.DepthStencilState);

	// color blend
	auto& blendDesc = m_Desc.BlendDesc;
	VkPipelineColorBlendStateCreateInfo colorBlendInfo{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, nullptr, 0 };
	colorBlendInfo.logicOpEnable = false;
	colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
	uint32 blendStateCount = blendDesc.BlendStates.Size();
	TempArray<VkPipelineColorBlendAttachmentState> states(blendStateCount);
	for(uint32 i=0; i<blendStateCount; ++i) {
		states[i] = ToAttachmentBlendState(blendDesc.BlendStates[i]);
	}
	for(uint32 i=0; i<4;++i) {
		colorBlendInfo.blendConstants[i] = blendDesc.BlendConst[i];
	}

	// dynamic state
	static VkDynamicState s_DynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicInfo{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, nullptr, 0 };
	dynamicInfo.dynamicStateCount = ArraySize(s_DynamicStates);
	dynamicInfo.pDynamicStates = s_DynamicStates;

	pipelineInfo.stageCount = shaderCount;
	pipelineInfo.pStages = shaderStages.Data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
	pipelineInfo.pTessellationState = &tessellationStateInfo;
	pipelineInfo.pViewportState = &viewportInfo;
	pipelineInfo.pRasterizationState = &rasterizationStateInfo;
	pipelineInfo.pMultisampleState = &multiSampleInfo;
	pipelineInfo.pDepthStencilState = &depthStencilInfo;
	pipelineInfo.pColorBlendState = &colorBlendInfo;
	pipelineInfo.pDynamicState = &dynamicInfo;
	pipelineInfo.layout = m_PipelineLayout;
	pipelineInfo.renderPass = pass;
	pipelineInfo.subpass = subPass;
	VK_CHECK(vkCreateGraphicsPipelines(RHIVulkan::GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline), "vkCreateGraphicsPipelines");
}

RHIVulkanComputePipelineState::RHIVulkanComputePipelineState(const RHIComputePipelineStateDesc& desc): RHIComputePipelineState(desc) {
	// create layout
	m_PipelineLayout = CreatePipelineLayout(desc.Layout);

	// create pipeline
	VkComputePipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO, nullptr, 0 };
	pipelineInfo.stage = dynamic_cast<RHIVulkanShader*>(m_Desc.Shader)->GetShaderStageInfo();
	pipelineInfo.layout = m_PipelineLayout;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = 0;
	vkCreateComputePipelines(RHIVulkan::GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline);
}

RHIVulkanComputePipelineState::~RHIVulkanComputePipelineState() {
	VkDevice device = RHIVulkan::GetDevice();
	vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
	vkDestroyPipeline(device, m_Pipeline, nullptr);
}

void RHIVulkanComputePipelineState::SetName(const char* name) {
	VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_PIPELINE, m_Pipeline, name);
}

RHIVulkanRenderPass::RHIVulkanRenderPass(const RHIRenderPassDesc& desc): RHIRenderPass(desc) {
}

RHIVulkanRenderPass::~RHIVulkanRenderPass() {
	DestroyHandle();
}

void RHIVulkanRenderPass::SetName(const char* name) {
	VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_RENDER_PASS, m_RenderPass, name);
}


void RHIVulkanRenderPass::DestroyHandle() {
	if(VK_NULL_HANDLE != m_RenderPass) {
		VkDevice device = RHIVulkan::GetDevice();
		vkDestroyRenderPass(device, m_RenderPass, nullptr);
		vkDestroyFramebuffer(device, m_Framebuffer, nullptr);
	}
}

void RHIVulkanRenderPass::CreateHandle(const VulkanLayoutMgr& layoutMgr) {
	VkRenderPassCreateInfo passInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, nullptr, 0 };
	uint32 attachmentCount = m_Desc.ColorTargets.Size() + !!m_Desc.DepthStencilTarget.Target;
	TempArray<VkAttachmentDescription> attachmentDescs(attachmentCount);
	TempArray<VkImageView> attachments(attachmentCount);
	TVector<VkAttachmentReference> colorRefs;
	uint32 i = 0;
	for (; i < m_Desc.ColorTargets.Size(); ++i) {
		const auto& rtInfo = m_Desc.ColorTargets[i];
		if(!rtInfo.Target) {
			continue;
		}
		attachmentDescs[i].format = ToVkFormat(rtInfo.Target->GetDesc().Format);
		attachmentDescs[i].samples = ToVkMultiSampleCount(1);//TODO
		attachmentDescs[i].loadOp = ToVkAttachmentLoadOp(rtInfo.LoadOp);
		attachmentDescs[i].storeOp = ToVkAttachmentStoreOp(rtInfo.StoreOp);
		attachmentDescs[i].initialLayout = layoutMgr.GetCurrentLayout(rtInfo.Target);
		attachmentDescs[i].finalLayout = layoutMgr.GetFinalLayout(rtInfo.Target);
		colorRefs.PushBack({ i, attachmentDescs[i].initialLayout });

		RHIVkTexture* vkTex = dynamic_cast<RHIVkTexture*>(rtInfo.Target);
		attachments[i] = vkTex->GetView();
	}

	// depth
	auto& depthInfo = m_Desc.DepthStencilTarget;
	VkAttachmentReference depthRef;
	if(depthInfo.Target) {
		attachmentDescs[i].format = ToVkFormat(depthInfo.Target->GetDesc().Format);
		attachmentDescs[i].samples = ToVkMultiSampleCount(1);//TODO
		attachmentDescs[i].loadOp = ToVkAttachmentLoadOp(depthInfo.DepthLoadOp);
		attachmentDescs[i].storeOp = ToVkAttachmentStoreOp(depthInfo.DepthStoreOp);
		attachmentDescs[i].stencilLoadOp = ToVkAttachmentLoadOp(depthInfo.StencilLoadOp);
		attachmentDescs[i].stencilStoreOp = ToVkAttachmentStoreOp(depthInfo.StencilStoreOp);
		attachmentDescs[i].initialLayout = layoutMgr.GetCurrentLayout(depthInfo.Target);
		attachmentDescs[i].finalLayout = layoutMgr.GetFinalLayout(depthInfo.Target);
		depthRef = { i, attachmentDescs[i].initialLayout };
		RHIVkTexture* vkTex = dynamic_cast<RHIVkTexture*>(depthInfo.Target);
		attachments[i] = vkTex->GetView();
		++i;
	}

	passInfo.attachmentCount = i;
	passInfo.pAttachments = attachmentDescs.Data();
	// TODO only ONE sub pass on PC, util mobile platforms are supported.
	passInfo.subpassCount = 1;
	VkSubpassDescription subpassDesc;
	subpassDesc.flags = 0;
	subpassDesc.inputAttachmentCount = 0;
	subpassDesc.pInputAttachments = nullptr;
	subpassDesc.colorAttachmentCount = colorRefs.Size();
	subpassDesc.pColorAttachments = colorRefs.Data();
	subpassDesc.pResolveAttachments = nullptr;
	subpassDesc.pDepthStencilAttachment = i > colorRefs.Size() ? &depthRef : nullptr;
	subpassDesc.preserveAttachmentCount = 0;
	subpassDesc.pResolveAttachments = nullptr;
	passInfo.pSubpasses = &subpassDesc;

	// subpass dependency
	VkSubpassDependency subpassDependency;
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency.dstSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.srcAccessMask = 0;
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	passInfo.dependencyCount = 1;
	passInfo.pDependencies = &subpassDependency;

	vkCreateRenderPass(RHIVulkan::GetDevice(), &passInfo, nullptr, &m_RenderPass);

	// create frame buffer
	VkFramebufferCreateInfo framebufferInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, nullptr, 0 };
	framebufferInfo.renderPass = m_RenderPass;
	framebufferInfo.attachmentCount = attachmentCount;
	framebufferInfo.pAttachments = attachments.Data();
	framebufferInfo.width = m_Desc.Size.w;
	framebufferInfo.height = m_Desc.Size.h;
	framebufferInfo.layers = 1;//TODO multi sampling
	vkCreateFramebuffer(RHIVulkan::GetDevice(), &framebufferInfo, nullptr, &m_Framebuffer);
}

RHIVulkanCommandBuffer::RHIVulkanCommandBuffer(VkCommandBuffer cmd, VkCommandPool pool) : m_VkCmd(cmd), m_Pool(pool), m_IsBegin(false) {
}

RHIVulkanCommandBuffer::~RHIVulkanCommandBuffer() {
	VkDevice device = RHIVulkan::GetDevice();
	if(m_IsBegin) {
		vkEndCommandBuffer(m_VkCmd);
	}
	vkFreeCommandBuffers(device, m_Pool, 1, &m_VkCmd);
}

void RHIVulkanCommandBuffer::BeginRenderPass(const RHIRenderPassDesc& passInfo) {
}

void RHIVulkanCommandBuffer::EndRenderPass() {
	vkCmdEndRenderPass(m_VkCmd);
}

void RHIVulkanCommandBuffer::CopyBufferToBuffer(RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, uint64 srcOffset, uint64 dstOffset, uint64 size) {
	VkBufferCopy copy{ srcOffset, dstOffset, size };
	vkCmdCopyBuffer(m_VkCmd, dynamic_cast<RHIVkBuffer*>(srcBuffer)->GetBuffer(), dynamic_cast<RHIVkBuffer*>(dstBuffer)->GetBuffer(), 1, &copy);
}

void RHIVulkanCommandBuffer::CopyBufferToTexture(RHIBuffer* buffer, RHITexture* texture, uint32 mipLevel, uint32 baseLayer, uint32 layerCount) {
	RHIVkTexture* vkTex = dynamic_cast<RHIVkTexture*>(texture);
	VkBufferImageCopy region;
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = ToImageAspectFlags(texture->GetDesc().Flags);
	region.imageSubresource.mipLevel = mipLevel;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = layerCount;
	region.imageOffset = { 0, 0, 0 };
	const USize3D size = vkTex->GetDesc().Size;
	region.imageExtent = { size.w, size.h, size.d };
	vkCmdCopyBufferToImage(m_VkCmd, ((RHIVkBuffer*)buffer)->GetBuffer(), vkTex->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void RHIVulkanCommandBuffer::CopyTextureToTexture(RHITexture* srcTex, RHITexture* dstTex, const RHITextureCopyRegion& region)
{
	VkImageBlit blit{};
	blit.srcSubresource.aspectMask = region.srcAspect;
	blit.srcSubresource.baseArrayLayer = region.srcBaseLayer;
	blit.srcSubresource.layerCount = region.srcLayerCount;
	memcpy(blit.srcOffsets, region.srcOffsets, sizeof(VkOffset3D) * 2);
	blit.dstSubresource.aspectMask = region.dstAspect;
	blit.dstSubresource.baseArrayLayer = region.dstBaseLayer;
	blit.dstSubresource.layerCount = region.dstLayerCount;
	memcpy(blit.dstOffsets, region.dstOffsets, sizeof(VkOffset3D) * 2);
	vkCmdBlitImage(m_VkCmd, ((RHIVkTexture*)srcTex)->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, ((RHIVkTexture*)srcTex)->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
}

void RHIVulkanCommandBuffer::TransitionTextureLayout(RHITexture* texture, RImageLayout oldLayout, RImageLayout newLayout, uint32 baseMipLevel, uint32 levelCount, uint32 baseLayer, uint32 layerCount, EImageAspectFlags aspect) {
	RHIVkTexture* vkTex = dynamic_cast<RHIVkTexture*>(texture);
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = (VkImageLayout)oldLayout;
	barrier.newLayout = (VkImageLayout)newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = vkTex->GetImage();
	barrier.subresourceRange.aspectMask = aspect;
	barrier.subresourceRange.baseMipLevel = baseMipLevel;
	barrier.subresourceRange.levelCount = levelCount;
	barrier.subresourceRange.baseArrayLayer = baseLayer;
	barrier.subresourceRange.layerCount = layerCount;
	VkPipelineStageFlags srcStage;
	VkPipelineStageFlags dstStage;
	GetPipelineBarrierStage(barrier.oldLayout, barrier.newLayout, barrier.srcAccessMask, barrier.dstAccessMask, srcStage, dstStage);
	vkCmdPipelineBarrier(m_VkCmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void RHIVulkanCommandBuffer::GenerateMipmap(RHITexture* texture, uint32 levelCount, EImageAspectFlags aspect, uint32 baseLayer, uint32 layerCount) {
	RHIVkTexture* vkTex = dynamic_cast<RHIVkTexture*>(texture);
	USize3D size = vkTex->GetDesc().Size;
	GenerateMipMap(m_VkCmd, vkTex->GetImage(), levelCount, size.w, size.h, aspect, baseLayer, layerCount);
}

void RHIVulkanCommandBuffer::BindGraphicsPipeline(RHIGraphicsPipelineState* pipeline) {
	RHIVulkanGraphicsPipelineState* vkPipeline = dynamic_cast<RHIVulkanGraphicsPipelineState*>(pipeline);
	vkCmdBindPipeline(m_VkCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline->GetPipelineHandle(m_CurrentPass, m_SubPass));
}

void RHIVulkanCommandBuffer::BindComputePipeline(RHIComputePipelineState* pipeline) {
	RHIVulkanComputePipelineState* vkPipeline = dynamic_cast<RHIVulkanComputePipelineState*>(pipeline);
	vkCmdBindPipeline(m_VkCmd, VK_PIPELINE_BIND_POINT_COMPUTE, vkPipeline->GetPipelineHandle());
}

void RHIVulkanCommandBuffer::BindVertexBuffer(RHIBuffer* buffer, uint32 first, uint64 offset) {
	VkBuffer vkBuffer = dynamic_cast<RHIVkBuffer*>(buffer)->GetBuffer();
	vkCmdBindVertexBuffers(m_VkCmd, first, 1, &vkBuffer, &offset);
}

void RHIVulkanCommandBuffer::BindIndexBuffer(RHIBuffer* buffer, uint64 offset) {
	vkCmdBindIndexBuffer(m_VkCmd, dynamic_cast<RHIVkBuffer*>(buffer)->GetBuffer(), offset, VK_INDEX_TYPE_UINT32);
}

void RHIVulkanCommandBuffer::Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstIndex, uint32 firstInstance) {
	vkCmdDraw(m_VkCmd, vertexCount, instanceCount, firstIndex, firstInstance);
}

void RHIVulkanCommandBuffer::DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance) {
	vkCmdDrawIndexed(m_VkCmd, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void RHIVulkanCommandBuffer::Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ) {
	vkCmdDispatch(m_VkCmd, groupCountX, groupCountY, groupCountZ);
}

void RHIVulkanCommandBuffer::ClearAttachment(EImageAspectFlags aspect, const float* color, const IRect& rect) {
	VkClearAttachment clearAttachment;
	clearAttachment.aspectMask = aspect;
	clearAttachment.clearValue.color.float32[0] = color[0];
	clearAttachment.clearValue.color.float32[1] = color[1];
	clearAttachment.clearValue.color.float32[2] = color[2];
	clearAttachment.clearValue.color.float32[3] = color[3];
	VkClearRect clearRect;
	clearRect.rect.extent.width = static_cast<uint32>(rect.w);
	clearRect.rect.extent.height = static_cast<uint32>(rect.h);
	clearRect.rect.offset.x = rect.x;
	clearRect.rect.offset.y = rect.y;
	clearRect.baseArrayLayer = 0;
	clearRect.layerCount = 1;
	vkCmdClearAttachments(m_VkCmd, 1, &clearAttachment, 1, &clearRect);
}

void RHIVulkanCommandBuffer::BeginDebugLabel(const char* msg, const float* color) {
	if (nullptr != vkCmdBeginDebugUtilsLabelEXT) {
		VkDebugUtilsLabelEXT labelInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, nullptr };
		labelInfo.pLabelName = msg;
		if (nullptr != color) {
			for (int i = 0; i < 4; ++i) {
				labelInfo.color[i] = color[i];
			}
		}
		vkCmdBeginDebugUtilsLabelEXT(m_VkCmd, &labelInfo);
	}
}

void RHIVulkanCommandBuffer::EndDebugLabel() {
	if (nullptr != vkCmdEndDebugUtilsLabelEXT) {
		vkCmdEndDebugUtilsLabelEXT(m_VkCmd);
	}
}

void RHIVulkanCommandBuffer::CmdBegin() {
	VkCommandBufferBeginInfo info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr };
	info.flags = 0;
	info.pInheritanceInfo = nullptr;
	vkBeginCommandBuffer(m_VkCmd, &info);
}

RHIVulkanShaderParameterSet::RHIVulkanShaderParameterSet(const RHIShaderBindingLayout& layout) {
	VkDescriptorSetLayout layoutHandle = VulkanDSMgr::Instance()->GetLayoutHandle(layout);
	VkDevice device = RHIVulkan::GetDevice();
	VkDescriptorSetAllocateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, nullptr };
}
