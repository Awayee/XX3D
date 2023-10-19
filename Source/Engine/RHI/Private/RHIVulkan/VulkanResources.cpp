#include "VulkanResources.h"
#include "RHIVulkan.h"
#include "VulkanBuilder.h"
#include "VulkanConverter.h"
#include "VulkanDescriptorSet.h"
#define VK_SET_OBJECT_NAME(type, handle, name) do{\
VkDebugUtilsObjectNameInfoEXT info{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, nullptr, type, reinterpret_cast<uint64>(handle), name};\
vkSetDebugUtilsObjectNameEXT(RHIVulkan::GetDevice(), &info);\
}while(0)

RHIVkBuffer::RHIVkBuffer(const RHIBufferDesc& desc, VkBuffer buffer, VulkanMem&& mem):
RHIBuffer(desc), m_Buffer(buffer), m_Mem(std::forward<VulkanMem>(mem)) {}

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

RHIVkTexture::RHIVkTexture(const RHITextureDesc& desc, VkImage image, VkImageView view): RHITexture(desc), m_Image(image), m_View(view) {
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

RHIVkTextureWithMem::RHIVkTextureWithMem(const RHITextureDesc& desc, VkImage image, VkImageView view, VulkanMem&& memory):
RHIVkTexture(desc, image, view), m_Mem(std::forward<VulkanMem>(memory)){}

RHIVkTextureWithMem::~RHIVkTextureWithMem() {
	m_Mem.Free();
}

RHIVkSampler::RHIVkSampler(const RHISamplerDesc& desc, VkSampler sampler): RHISampler(desc), m_Sampler(sampler){}

RHIVkSampler::~RHIVkSampler() {
	VkDevice device = RHIVulkan::GetDevice();
	vkDestroySampler(device, m_Sampler, nullptr);
}

void RHIVkSampler::SetName(const char* name) {
	VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_SAMPLER, m_Sampler, name);
}

RHIVkFence::RHIVkFence(VkFence fence): m_Handle(fence) {
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

RHIVulkanRenderPass::RHIVulkanRenderPass(const RHIRenderPassDesc& desc): RHIRenderPass(desc),
m_RenderPass(VK_NULL_HANDLE), m_Framebuffer(VK_NULL_HANDLE){}

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
	framebufferInfo.width = m_Desc.RenderSize.w;
	framebufferInfo.height = m_Desc.RenderSize.h;
	framebufferInfo.layers = 1;//TODO multi sampling
	vkCreateFramebuffer(RHIVulkan::GetDevice(), &framebufferInfo, nullptr, &m_Framebuffer);
}