#include "VulkanPipeline.h"
#include "Core/Public/Algorithm.h"
#include "VulkanConverter.h"
#include "Core/Public/Log.h"
#include "VulkanResources.h"
#include "VulkanDevice.h"

inline uint64 GetLayoutHash(const RHIShaderParamSetLayout& bindingLayout) {
	const uint64* dataU64 = reinterpret_cast<const uint64*>(bindingLayout.Data());
	return DataArrayHash(dataU64, bindingLayout.Size());
}

VkDescriptorSetLayout VulkanDescriptorSetMgr::GetLayoutHandle(const RHIShaderParamSetLayout& layout) {
	uint64 hs = GetLayoutHash(layout);
	auto iter = m_LayoutMap.find(hs);
	if(iter!= m_LayoutMap.end()) {
		return iter->second;
	}

	VkDescriptorSetLayoutCreateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0 };
	uint32 bindingCount = layout.Size();
	TFixedArray<VkDescriptorSetLayoutBinding> bindings(bindingCount);
	for(uint32 i=0; i<bindingCount; ++i) {
		auto& binding = bindings[i];
		binding = {};
		binding.binding = i;
		binding.descriptorType = ToVkDescriptorType(layout[i].Type);
		binding.descriptorCount = layout[i].Count;
		binding.stageFlags = ToVkShaderStageFlags(layout[i].StageFlags);
	}
	info.bindingCount = bindingCount;
	info.pBindings = bindings.Data();
	VkDescriptorSetLayout layoutHandle;
	vkCreateDescriptorSetLayout(m_Device->GetDevice(), &info, nullptr, &layoutHandle);
	m_LayoutMap.emplace(hs, layoutHandle);
	return layoutHandle;
}

VkDescriptorPool VulkanDescriptorSetMgr::GetPool() {
	return m_Pools[0];
}

VkDescriptorSet VulkanDescriptorSetMgr::AllocateDescriptorSet(VkDescriptorSetLayout layout) {
	VkDescriptorSet set;
	VkDescriptorSetAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, nullptr };
	allocateInfo.descriptorPool = GetPool();
	allocateInfo.descriptorSetCount = 1;
	allocateInfo.pSetLayouts = &layout;
	vkAllocateDescriptorSets(m_Device->GetDevice(), &allocateInfo, &set);
	return set;
}

void VulkanDescriptorSetMgr::FreeDescriptorSet(VkDescriptorSet set) {
	vkFreeDescriptorSets(m_Device->GetDevice(), GetPool(), 1, &set);
}

void VulkanDescriptorSetMgr::AllocateDescriptorSets(TConstArrayView<VkDescriptorSetLayout> layouts, TArrayView<VkDescriptorSet> sets) {
	VkDescriptorSetAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, nullptr };
	allocateInfo.descriptorPool = GetPool();
	allocateInfo.descriptorSetCount = layouts.Size();
	allocateInfo.pSetLayouts = layouts.Data();
	vkAllocateDescriptorSets(m_Device->GetDevice(), &allocateInfo, sets.Data());
}

void VulkanDescriptorSetMgr::FreeDescriptorSets(TArrayView<VkDescriptorSet> sets) {
	vkFreeDescriptorSets(m_Device->GetDevice(), GetPool(), sets.Size(), sets.Data());
	for(VkDescriptorSet& set: sets) {
		set = VK_NULL_HANDLE;
	}
}

void VulkanDescriptorSetMgr::Update() {
}

VkDescriptorPool VulkanDescriptorSetMgr::AddPool() {
	VkDescriptorPoolCreateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, nullptr};
	info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	constexpr uint8 bindingCount = static_cast<uint8>(EBindingType::MaxNum);
	static const uint32 s_CountPerType[bindingCount] = {8, 128, 64, 128, 16, 128};
	VkDescriptorPoolSize sizes[bindingCount];
	uint32 allCount = 0;
	for(uint8 i=0; i<static_cast<uint8>(EBindingType::MaxNum); ++i) {
		sizes[i].type = ToVkDescriptorType(static_cast<EBindingType>(i));
		sizes[i].descriptorCount = s_CountPerType[i];
		allCount += sizes[i].descriptorCount;
	}
	info.poolSizeCount = bindingCount;
	info.pPoolSizes = sizes;
	info.maxSets = allCount;
	VkDescriptorPool handle;
	vkCreateDescriptorPool(m_Device->GetDevice(), &info, nullptr, &handle);
	m_Pools.PushBack(handle);
	return handle;
}

VulkanDescriptorSetMgr::VulkanDescriptorSetMgr(VulkanDevice* device) : m_Device(device) {
	AddPool();
}

VulkanDescriptorSetMgr::~VulkanDescriptorSetMgr() {
	for (auto& [hs, layoutHandle] : m_LayoutMap) {
		vkDestroyDescriptorSetLayout(m_Device->GetDevice(), layoutHandle, nullptr);
	}
	for (VkDescriptorPool& pool : m_Pools) {
		vkDestroyDescriptorPool(m_Device->GetDevice(), pool, nullptr);
		pool = VK_NULL_HANDLE;
	}
}

VulkanPipelineDescriptorSetCache::VulkanPipelineDescriptorSetCache(VulkanDevice* device, TConstArrayView<VkDescriptorSetLayout> layouts):
	m_Device(device),
	m_Layouts(layouts) {
	// Allocate descriptor sets
	m_DescriptorSets.Resize(m_Layouts.Size());
	m_Device->GetDescriptorMgr()->AllocateDescriptorSets(m_Layouts, m_DescriptorSets);
}

VulkanPipelineDescriptorSetCache::~VulkanPipelineDescriptorSetCache() {
	m_Device->GetDescriptorMgr()->FreeDescriptorSets(m_DescriptorSets);
}

void VulkanPipelineDescriptorSetCache::SetParameter(uint32 setIndex, uint32 bindIndex, const RHIShaderParam& parameter) {
	ASSERT(setIndex < m_Layouts.Size(), "[VulkanPipelineDescriptorSetCache::SetParameter] setIndex out of range!");
	union WriteInfo {
		VkDescriptorBufferInfo BufferInfo;
		VkDescriptorImageInfo ImageInfo;
	} writeInfo {};
	VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr };
	write.dstSet = m_DescriptorSets[setIndex];
	write.dstBinding = bindIndex;
	write.dstArrayElement = parameter.ArrayIndex;
	write.descriptorCount = 1;
	write.descriptorType = ToVkDescriptorType(parameter.Type);
	switch (parameter.Type) {
	case EBindingType::UniformBuffer:
	case EBindingType::StorageBuffer: {
		VulkanRHIBuffer* buffer = (VulkanRHIBuffer*)parameter.Data.Buffer;
		writeInfo.BufferInfo.buffer = buffer->GetBuffer();
		writeInfo.BufferInfo.offset = parameter.Data.Offset;
		writeInfo.BufferInfo.range = parameter.Data.Size;
		write.pBufferInfo = &writeInfo.BufferInfo;
	}break;
	case EBindingType::Sampler: {
		writeInfo.ImageInfo.sampler = ((VulkanRHISampler*)parameter.Data.Sampler)->GetSampler();
		write.pImageInfo = &writeInfo.ImageInfo;
	}break;
	case EBindingType::TextureSampler:
		writeInfo.ImageInfo.sampler = ((VulkanRHISampler*)parameter.Data.Sampler)->GetSampler();
	case EBindingType::Texture:
	case EBindingType::StorageTexture:{
		VulkanRHITexture* texture = (VulkanRHITexture*)(parameter.Data.Texture);
		writeInfo.ImageInfo.imageView = texture->GetView(parameter.Data.SRVType, parameter.Data.TextureSub); // TODO more view types
		writeInfo.ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		write.pImageInfo = &writeInfo.ImageInfo;
	}break;
	}
	vkUpdateDescriptorSets(m_Device->GetDevice(), 1, &write, 0, nullptr);
}

TConstArrayView<VkDescriptorSet> VulkanPipelineDescriptorSetCache::GetDescriptorSets() const {
	return m_DescriptorSets;
}


VulkanRHIGraphicsPipelineState::VulkanRHIGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc, VulkanDevice* device) :RHIGraphicsPipelineState(desc), m_Device(device) {
	// Create pipeline layout
	{
		uint32 layoutCount = desc.Layout.Size();
		m_SetLayouts.Resize(layoutCount);
		for (uint32 i = 0; i < layoutCount; ++i) {
			m_SetLayouts[i] = m_Device->GetDescriptorMgr()->GetLayoutHandle(desc.Layout[i]);
		}
		VkPipelineLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0 };
		layoutInfo.setLayoutCount = m_SetLayouts.Size();
		layoutInfo.pSetLayouts = m_SetLayouts.Data();
		VK_ASSERT(vkCreatePipelineLayout(m_Device->GetDevice(), &layoutInfo, nullptr, &m_PipelineLayout), "PipelineLayout");
	}

	// Create pipeline handle
	{
		VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
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
		colorBlendInfo.attachmentCount = blendStateCount;
		colorBlendInfo.pAttachments = states.Data();
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
		uint32 colorAttachmentCount = desc.ColorFormats.Size();
		TFixedArray<VkFormat> colorAttachmentFormats(colorAttachmentCount);
		for (uint32 i = 0; i < colorAttachmentCount; ++i) {
			colorAttachmentFormats[i] = ToVkFormat(desc.ColorFormats[i]);
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
	vkDestroyPipelineLayout(m_Device->GetDevice(), m_PipelineLayout, nullptr);
	vkDestroyPipeline(m_Device->GetDevice(), m_Pipeline, nullptr);
}

void VulkanRHIGraphicsPipelineState::SetName(const char* name) {
	VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_PIPELINE, m_Pipeline, name);
}

VulkanRHIComputePipelineState::VulkanRHIComputePipelineState(const RHIComputePipelineStateDesc& desc, VulkanDevice* device) : RHIComputePipelineState(desc), m_Device(device) {
	// Create pipeline layout
	{
		uint32 layoutCount = desc.Layout.Size();
		m_SetLayouts.Resize(layoutCount);
		for (uint32 i = 0; i < layoutCount; ++i) {
			m_SetLayouts[i] = m_Device->GetDescriptorMgr()->GetLayoutHandle(desc.Layout[i]);
		}
		VkPipelineLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0 };
		layoutInfo.setLayoutCount = m_SetLayouts.Size();
		layoutInfo.pSetLayouts = m_SetLayouts.Data();
		VK_ASSERT(vkCreatePipelineLayout(m_Device->GetDevice(), &layoutInfo, nullptr, &m_PipelineLayout), "PipelineLayout");
	}

	// create pipeline
	{
		VkComputePipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO, nullptr, 0 };
		pipelineInfo.stage = static_cast<VulkanRHIShader*>(m_Desc.Shader)->GetShaderStageInfo();
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

VulkanPipelineStateContainer::VulkanPipelineStateContainer(VulkanDevice* device) : m_Device(device){
}

void VulkanPipelineStateContainer::BindPipelineState(VulkanRHIGraphicsPipelineState* pso) {
	PipelineData& data = m_Pipelines.EmplaceBack();
	data.DescriptorSetCache.Reset(new VulkanPipelineDescriptorSetCache(m_Device, pso->GetSetLayouts()));
	data.PipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	data.GraphicsPipelineState = pso;
}

void VulkanPipelineStateContainer::BindPipelineState(VulkanRHIComputePipelineState* pso) {
	PipelineData& data = m_Pipelines.EmplaceBack();
	data.DescriptorSetCache.Reset(new VulkanPipelineDescriptorSetCache(m_Device, pso->GetSetLayouts()));
	data.PipelineBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
	data.ComputePipelineState = pso;
}

VulkanPipelineDescriptorSetCache* VulkanPipelineStateContainer::GetCurrentDescriptorSetCache() {
	return m_Pipelines.IsEmpty() ? nullptr : m_Pipelines.Back().DescriptorSetCache.Get();
}

VulkanRHIComputePipelineState* VulkanPipelineStateContainer::GetCurrentComputePipelineState() {
	if(!m_Pipelines.IsEmpty()) {
		auto& data = m_Pipelines.Back();
		if(VK_PIPELINE_BIND_POINT_COMPUTE == data.PipelineBindPoint) {
			return data.ComputePipelineState;
		}
	}
	return nullptr;
}

VulkanRHIGraphicsPipelineState* VulkanPipelineStateContainer::GetCurrentGraphicsPipelineState() {
	if (!m_Pipelines.IsEmpty()) {
		auto& data = m_Pipelines.Back();
		if (VK_PIPELINE_BIND_POINT_GRAPHICS == data.PipelineBindPoint) {
			return data.GraphicsPipelineState;
		}
	}
	return nullptr;
}

void VulkanPipelineStateContainer::Reset() {
	m_Pipelines.Reset();
}
