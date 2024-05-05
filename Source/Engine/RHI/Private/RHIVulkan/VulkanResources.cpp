#include "VulkanResources.h"
#include "RHIVulkan.h"
#include "VulkanBuilder.h"
#include "VulkanConverter.h"
#include "VulkanDescriptorSet.h"
#define VK_SET_OBJECT_NAME(type, handle, name) do{\
VkDebugUtilsObjectNameInfoEXT info{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, nullptr, type, reinterpret_cast<uint64>(handle), name};\
vkSetDebugUtilsObjectNameEXT(RHIVulkan::GetDevice(), &info);\
}while(0)

RHIVulkanBufferWithMem::RHIVulkanBufferWithMem(const RHIBufferDesc& desc, VkBuffer buffer, VulkanMem&& mem):
RHIBuffer(desc), m_Buffer(buffer), m_Mem(std::forward<VulkanMem>(mem)) {}

RHIVulkanBufferWithMem::~RHIVulkanBufferWithMem() {
	VkDevice device = RHIVulkan::GetDevice();
	vkDestroyBuffer(device, m_Buffer, nullptr);
	m_Mem.Free();
}

void RHIVulkanBufferWithMem::SetName(const char* name) {
	VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_BUFFER, m_Buffer, name);
}

void RHIVulkanBufferWithMem::UpdateData(const void* data, size_t byteSize) {
	void* mappedData = m_Mem.Map();
	memcpy(mappedData, data, byteSize);
	m_Mem.Unmap();
}

RHIVulkanTexture::RHIVulkanTexture(const RHITextureDesc& desc, VkImage image, VkImageView view): RHITexture(desc), m_Image(image), m_View(view) {
}

RHIVulkanTexture::~RHIVulkanTexture() {
	VkDevice device = RHIVulkan::GetDevice();
	vkDestroyImage(device, m_Image, nullptr);
	vkDestroyImageView(device, m_View, nullptr);
}

void RHIVulkanTexture::SetName(const char* name) {
	VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_IMAGE, m_Image, name);
	VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_IMAGE_VIEW, m_View, name);
}

void RHIVulkanTexture::UpdateData(RHITextureOffset offset, USize3D size, const void* data) {
	// TODO
}

RHIVulkanTextureWithMem::RHIVulkanTextureWithMem(const RHITextureDesc& desc, VkImage image, VkImageView view, VulkanMem&& memory):
RHIVulkanTexture(desc, image, view), m_Mem(std::forward<VulkanMem>(memory)){}

RHIVulkanTextureWithMem::~RHIVulkanTextureWithMem() {
	m_Mem.Free();
}

RHIVulkanSampler::RHIVulkanSampler(const RHISamplerDesc& desc, VkSampler sampler): RHISampler(desc), m_Sampler(sampler){}

RHIVulkanSampler::~RHIVulkanSampler() {
	VkDevice device = RHIVulkan::GetDevice();
	vkDestroySampler(device, m_Sampler, nullptr);
}

void RHIVulkanSampler::SetName(const char* name) {
	VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_SAMPLER, m_Sampler, name);
}

RHIVulkanFence::RHIVulkanFence(VkFence fence): m_Handle(fence) {
}


RHIVulkanFence::~RHIVulkanFence() {
	VkDevice device = RHIVulkan::GetDevice();
	vkDestroyFence(device, m_Handle, nullptr);
}

void RHIVulkanFence::Wait() {
	vkWaitForFences(RHIVulkan::GetDevice(), 1, &m_Handle, 1, WAIT_MAX);
}

void RHIVulkanFence::Reset() {
	vkResetFences(RHIVulkan::GetDevice(), 1, &m_Handle);
}

void RHIVulkanFence::SetName(const char* name) {
	VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_FENCE, m_Handle, name);
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

RHIVulkanGraphicsPipelineState::RHIVulkanGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc, VkPipelineLayout layout):RHIGraphicsPipelineState(desc) {
	m_PipelineLayout = layout;
}

RHIVulkanGraphicsPipelineState::~RHIVulkanGraphicsPipelineState() {
	Release();
}

void RHIVulkanGraphicsPipelineState::SetName(const char* name) {
	m_Name = name;
}

VkPipeline RHIVulkanGraphicsPipelineState::GetPipelineHandle(VkRenderPass pass, uint32 subPass) {
	if(m_TargetRenderPass == pass && m_TargetSubPass == subPass && m_Pipeline) {
		return m_Pipeline;
	}
	if(m_Pipeline) {
		vkDestroyPipeline(RHIVulkan::GetDevice(), m_Pipeline, nullptr);
	}
	CreatePipelineHandle(pass, subPass);
	return m_Pipeline;
}

void RHIVulkanGraphicsPipelineState::CreatePipelineHandle(VkRenderPass pass, uint32 subPass) {
	VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, nullptr, 0 };
	// shader stage
	uint32 shaderCount = !!m_Desc.VertexShader + !!m_Desc.PixelShader;
	TFixedArray<VkPipelineShaderStageCreateInfo> shaderStages(shaderCount);
	shaderCount = 0;
	if(m_Desc.VertexShader) {
		shaderStages[shaderCount++] = static_cast<RHIVulkanShader*>(m_Desc.VertexShader)->GetShaderStageInfo();
	}
	if(m_Desc.PixelShader) {
		shaderStages[shaderCount++] = static_cast<RHIVulkanShader*>(m_Desc.PixelShader)->GetShaderStageInfo();
	}

	// vertex input
	auto& vertexInput = m_Desc.VertexInput;
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, nullptr, 0 };
	const uint32 attrCount = vertexInput.Attributes.Size();
	TFixedArray<VkVertexInputAttributeDescription> viAttrs(attrCount);
	for (uint32 i = 0; i < attrCount; ++i) {
		viAttrs[i].format = ToVkFormat(vertexInput.Attributes[i].Format);
		viAttrs[i].binding = vertexInput.Attributes[i].Binding;
		viAttrs[i].location = i;
		viAttrs[i].offset = vertexInput.Attributes[i].Offset;
	}
	vertexInputInfo.vertexAttributeDescriptionCount = attrCount;
	vertexInputInfo.pVertexAttributeDescriptions = viAttrs.Data();
	const uint32 bindingCount = vertexInput.Bindings.Size();
	TFixedArray<VkVertexInputBindingDescription> viBindings(bindingCount);
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
	TFixedArray<VkPipelineColorBlendAttachmentState> states(blendStateCount);
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

	m_TargetRenderPass = pass;
	m_TargetSubPass = subPass;
}

void RHIVulkanGraphicsPipelineState::Release() {
	VkDevice device = RHIVulkan::GetDevice();
	vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
	vkDestroyPipeline(device, m_Pipeline, nullptr);
}

RHIVulkanComputePipelineState::RHIVulkanComputePipelineState(const RHIComputePipelineStateDesc& desc, VkPipelineLayout layout): RHIComputePipelineState(desc) {
	// create layout
	m_PipelineLayout = layout;

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


VulkanImageLayoutWrap* VulkanImageLayoutMgr::GetLayout(RHITexture* tex) {
	const VkImage image = dynamic_cast<RHIVulkanTexture*>(tex)->GetImage();
	return &m_ImageLayoutMap[image];
}

const VulkanImageLayoutWrap* VulkanImageLayoutMgr::GetLayout(RHITexture* tex) const {
	VkImage image = dynamic_cast<RHIVulkanTexture*>(tex)->GetImage();
	if(auto iter = m_ImageLayoutMap.find(image); iter!=m_ImageLayoutMap.end()) {
		return &iter->second;
	}
	return nullptr;
}


RHIVulkanRenderPass::RHIVulkanRenderPass(const RHIRenderPassDesc& desc): RHIRenderPass(desc), m_RenderPass(VK_NULL_HANDLE), m_Framebuffer(VK_NULL_HANDLE){}

RHIVulkanRenderPass::~RHIVulkanRenderPass() {
	DestroyHandle();
}

void RHIVulkanRenderPass::SetName(const char* name) {
	VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_RENDER_PASS, m_RenderPass, name);
}

void RHIVulkanRenderPass::ResolveImageLayout(const VulkanImageLayoutMgr* layoutMgr) {
	if(m_ImageLayoutMgr != layoutMgr) {
		m_ImageLayoutMgr = layoutMgr;
		DestroyHandle();
		CreateHandle();
	}
}


void RHIVulkanRenderPass::DestroyHandle() {
	if(VK_NULL_HANDLE != m_RenderPass) {
		VkDevice device = RHIVulkan::GetDevice();
		vkDestroyRenderPass(device, m_RenderPass, nullptr);
		vkDestroyFramebuffer(device, m_Framebuffer, nullptr);
	}
}

void RHIVulkanRenderPass::CreateHandle() {
	ASSERT(m_ImageLayoutMgr, "RHIVulkanRenderPass::CreateHandle() null image layout mgr!");
	VkRenderPassCreateInfo passInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, nullptr, 0 };
	uint32 attachmentCount = m_Desc.ColorTargets.Size() + !!m_Desc.DepthStencilTarget.Target;
	TFixedArray<VkAttachmentDescription> attachmentDescs(attachmentCount);
	TFixedArray<VkImageView> attachments(attachmentCount);
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
		const auto* layoutWarp = m_ImageLayoutMgr->GetLayout(rtInfo.Target);
		ASSERT(layoutWarp, "");
		attachmentDescs[i].initialLayout = layoutWarp->CurrentLayout;
		attachmentDescs[i].finalLayout = layoutWarp->FinalLayout;
		colorRefs.PushBack({ i, attachmentDescs[i].initialLayout });

		RHIVulkanTexture* vkTex = dynamic_cast<RHIVulkanTexture*>(rtInfo.Target);
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
		const auto* layoutWarp = m_ImageLayoutMgr->GetLayout(depthInfo.Target);
		ASSERT(layoutWarp, "");
		attachmentDescs[i].initialLayout = layoutWarp->CurrentLayout;
		attachmentDescs[i].finalLayout = layoutWarp->FinalLayout;
		depthRef = { i, attachmentDescs[i].initialLayout };
		RHIVulkanTexture* vkTex = dynamic_cast<RHIVulkanTexture*>(depthInfo.Target);
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
	framebufferInfo.width = m_Desc.RenderSize.w;
	framebufferInfo.height = m_Desc.RenderSize.h;
	framebufferInfo.layers = 1;//TODO multi sampling
	vkCreateFramebuffer(RHIVulkan::GetDevice(), &framebufferInfo, nullptr, &m_Framebuffer);
}