#include "VulkanResources.h"

#include "VulkanCommand.h"
#include "VulkanRHI.h"
#include "VulkanConverter.h"
#include "VulkanDescriptorSet.h"
#include "VulkanDevice.h"
#include "VulkanUploader.h"

#define VK_SET_OBJECT_NAME(type, handle, name) do{\
VkDebugUtilsObjectNameInfoEXT info{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, nullptr, type, reinterpret_cast<uint64>(handle), name};\
vkSetDebugUtilsObjectNameEXT(m_Device->GetDevice(), &info);\
}while(0)

VulkanRHIBuffer::VulkanRHIBuffer(const RHIBufferDesc& desc, VulkanDevice* device, BufferAllocation&& alloc):
RHIBuffer(desc), m_Device(device), m_Allocation(std::forward<BufferAllocation>(alloc)) {}

VulkanRHIBuffer::~VulkanRHIBuffer() {
	m_Device->GetMemoryMgr()->FreeBufferMemory(m_Allocation);
}

void VulkanRHIBuffer::SetName(const char* name) {
	
	VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_BUFFER, GetBuffer(), name);
}

void VulkanRHIBuffer::UpdateData(const void* data, uint32 byteSize) {
	const EBufferFlags bufferFlags = GetDesc().Flags;
	if(bufferFlags & EBufferFlagBit::BUFFER_FLAG_UNIFORM) {
		void* mappedData = m_Allocation.Map();
		memcpy(mappedData, data, byteSize);
		m_Allocation.Unmap();
	}
	else {
		ASSERT(bufferFlags & EBufferFlagBit::BUFFER_FLAG_COPY_DST, "[HIVulkanBufferWithMem::UpdateData] Buffer is not used with Copy Dst!");
		VulkanStagingBuffer* staging = m_Device->GetUploader()->AcquireBuffer(byteSize);
		void* mappedPointer = staging->Map();
		memcpy(mappedPointer, data, byteSize);
		staging->Unmap();
		// TODO replace with vulkan functions
		auto cmd = m_Device->GetCommandMgr()->GetUploadCmd();
		cmd->CopyBufferToBuffer(staging, this, 0, 0, byteSize);
	}
}

VulkanRHITexture::VulkanRHITexture(const RHITextureDesc& desc, VulkanDevice* device, VkImage image, VkImageView view, ImageAllocation&& alloc):
RHITexture(desc), m_Device(device), m_Image(image), m_View(view), m_Allocation(MoveTemp(alloc)) {}

VulkanRHITexture::~VulkanRHITexture() {
	vkDestroyImage(m_Device->GetDevice(), m_Image, nullptr);
	vkDestroyImageView(m_Device->GetDevice(), m_View, nullptr);
	m_Device->GetMemoryMgr()->FreeImageMemory(m_Allocation);
}

void VulkanRHITexture::SetName(const char* name) {
	VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_IMAGE, m_Image, name);
	VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_IMAGE_VIEW, m_View, name);
}

void VulkanRHITexture::UpdateData(RHITextureOffset offset, uint32 byteSize, const void* data) {
	ASSERT(GetDesc().Flags & ETextureFlagBit::TEXTURE_FLAG_CPY_DST, "");
	VulkanStagingBuffer* staging = m_Device->GetUploader()->AcquireBuffer(byteSize);
	void* mappedPointer = staging->Map();
	memcpy(mappedPointer, data, byteSize);
	staging->Unmap();
	// TODO replace with vulkan functions
	auto cmd = m_Device->GetCommandMgr()->GetUploadCmd();
	RHITextureSubDesc desc{ offset.MipLevel, 1, offset.ArrayLayer, 1 };
	cmd->PipelineBarrier(this, desc, EResourceState::Unknown, EResourceState::TransferDst);
	cmd->CopyBufferToTexture(staging, this, offset.MipLevel, offset.ArrayLayer, 1);
	cmd->PipelineBarrier(this, desc, EResourceState::TransferDst, EResourceState::ShaderResourceView);
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
	vkWaitForFences(m_Device->GetDevice(), 1, &m_Handle, 1, WAIT_MAX);
}

void VulkanRHIFence::Reset() {
	vkResetFences(m_Device->GetDevice(), 1, &m_Handle);
}

void VulkanRHIFence::SetName(const char* name) {
	VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_FENCE, m_Handle, name);
}

VulkanRHIShader::VulkanRHIShader(EShaderStageFlagBit type, const char* code, size_t codeSize, const char* funcName, VulkanDevice* device): RHIShader(type), m_Device(device) {
	VkShaderModuleCreateInfo shaderInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr, 0, codeSize, reinterpret_cast<const uint32*>(code)};
	vkCreateShaderModule(m_Device->GetDevice(), &shaderInfo, nullptr, &m_ShaderModule);
	m_EntryName = funcName;
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

VulkanRHIGraphicsPipelineState::VulkanRHIGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc, VulkanDevice* device):RHIGraphicsPipelineState(desc), m_Device(device) {
	// Create pipeline layout
	{
		uint32 layoutCount = desc.Layout.Size();
		TFixedArray<VkDescriptorSetLayout> layouts(layoutCount);
		for (uint32 i = 0; i < layoutCount; ++i) {
			layouts[i] = m_Device->GetDescriptorMgr()->GetLayoutHandle(desc.Layout[i]);
		}
		VkPipelineLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0 };
		layoutInfo.setLayoutCount = layoutCount;
		layoutInfo.pSetLayouts = layouts.Data();
		VK_ASSERT(vkCreatePipelineLayout(m_Device->GetDevice(), &layoutInfo, nullptr, &m_PipelineLayout), "PipelineLayout");
	}

	// Create pipeline handle
	{
		VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
		// shader stage
		uint32 shaderCount = !!m_Desc.VertexShader + !!m_Desc.PixelShader;
		TFixedArray<VkPipelineShaderStageCreateInfo> shaderStages(shaderCount);
		shaderCount = 0;
		if (m_Desc.VertexShader) {
			shaderStages[shaderCount++] = static_cast<VulkanRHIShader*>(m_Desc.VertexShader)->GetShaderStageInfo();
		}
		if (m_Desc.PixelShader) {
			shaderStages[shaderCount++] = static_cast<VulkanRHIShader*>(m_Desc.PixelShader)->GetShaderStageInfo();
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
		for (uint32 i = 0; i < bindingCount; ++i) {
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
		for (uint32 i = 0; i < blendStateCount; ++i) {
			states[i] = ToAttachmentBlendState(blendDesc.BlendStates[i]);
		}
		for (uint32 i = 0; i < 4; ++i) {
			colorBlendInfo.blendConstants[i] = blendDesc.BlendConst[i];
		}

		// dynamic state
		static VkDynamicState s_DynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicInfo{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, nullptr, 0 };
		dynamicInfo.dynamicStateCount = ArraySize(s_DynamicStates);
		dynamicInfo.pDynamicStates = s_DynamicStates;

		// rendering create info
		VkPipelineRenderingCreateInfo renderingCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO, nullptr };
		renderingCreateInfo.viewMask = 0;// TODO multi view
		uint32 colorAttachmentCount = desc.RenderTargetFormats.Size();
		TFixedArray<VkFormat> colorAttachmentFormats(colorAttachmentCount);
		for(uint32 i=0; i<colorAttachmentCount; ++i) {
			colorAttachmentFormats[i] = ToVkFormat(desc.RenderTargetFormats[i]);
		}
		renderingCreateInfo.colorAttachmentCount = colorAttachmentCount;
		renderingCreateInfo.pColorAttachmentFormats = colorAttachmentFormats.Data();
		renderingCreateInfo.depthAttachmentFormat = ToVkFormat(desc.DepthStencilFormat);
		renderingCreateInfo.stencilAttachmentFormat = renderingCreateInfo.depthAttachmentFormat;

		pipelineInfo.pNext = &renderingCreateInfo;
		pipelineInfo.flags = 0u;
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

		VK_ASSERT(vkCreateGraphicsPipelines(m_Device->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline), "vkCreateGraphicsPipelines");
	}
}

VulkanRHIGraphicsPipelineState::~VulkanRHIGraphicsPipelineState() {
	vkDestroyPipeline(m_Device->GetDevice(), m_Pipeline, nullptr);
	vkDestroyPipelineLayout(m_Device->GetDevice(), m_PipelineLayout, nullptr);
}

void VulkanRHIGraphicsPipelineState::SetName(const char* name) {
	VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_PIPELINE, m_Pipeline, name);
}

VulkanRHIComputePipelineState::VulkanRHIComputePipelineState(const RHIComputePipelineStateDesc& desc, VulkanDevice* device): RHIComputePipelineState(desc), m_Device(device) {
	// create layout
	{
		uint32 layoutCount = desc.Layout.Size();
		TFixedArray<VkDescriptorSetLayout> layouts(layoutCount);
		for (uint32 i = 0; i < layoutCount; ++i) {
			layouts[i] = m_Device->GetDescriptorMgr()->GetLayoutHandle(desc.Layout[i]);
		}
		VkPipelineLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0 };
		layoutInfo.setLayoutCount = layoutCount;
		layoutInfo.pSetLayouts = layouts.Data();
		VK_ASSERT(vkCreatePipelineLayout(m_Device->GetDevice(), &layoutInfo, nullptr, &m_PipelineLayout), "vkCreatePipelineLayout");
	}

	// create pipeline
	{
		VkComputePipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO, nullptr, 0 };
		pipelineInfo.stage = dynamic_cast<VulkanRHIShader*>(m_Desc.Shader)->GetShaderStageInfo();
		pipelineInfo.layout = m_PipelineLayout;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = 0;
		vkCreateComputePipelines(m_Device->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline);
	}
}

VulkanRHIComputePipelineState::~VulkanRHIComputePipelineState() {
	vkDestroyPipelineLayout(m_Device->GetDevice(), m_PipelineLayout, nullptr);
	vkDestroyPipeline(m_Device->GetDevice(), m_Pipeline, nullptr);
}

void VulkanRHIComputePipelineState::SetName(const char* name) {
	VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_PIPELINE, m_Pipeline, name);
}


VulkanImageLayoutWrap* VulkanImageLayoutMgr::GetLayout(RHITexture* tex) {
	const VkImage image = dynamic_cast<VulkanRHITexture*>(tex)->GetImage();
	return &m_ImageLayoutMap[image];
}

const VulkanImageLayoutWrap* VulkanImageLayoutMgr::GetLayout(RHITexture* tex) const {
	VkImage image = dynamic_cast<VulkanRHITexture*>(tex)->GetImage();
	if(auto iter = m_ImageLayoutMap.find(image); iter!=m_ImageLayoutMap.end()) {
		return &iter->second;
	}
	return nullptr;
}