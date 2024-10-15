#include "VulkanPipeline.h"
#include "Core/Public/Algorithm.h"
#include "VulkanConverter.h"
#include "Core/Public/Log.h"
#include "VulkanResources.h"
#include "VulkanDevice.h"
#include "VulkanMemory.h"

inline uint64 GetLayoutHash(const RHIShaderParamSetLayout& bindingLayout) {
	const uint32* dataU32 = reinterpret_cast<const uint32*>(bindingLayout.Data());
	return DataArrayHash(dataU32, bindingLayout.Size());
}

inline bool BindingIsBuffer(EBindingType type) {
	return type == EBindingType::UniformBuffer ||
		type == EBindingType::StorageBuffer;
}

inline bool BindingIsImage(EBindingType type) {
	return type == EBindingType::Texture ||
		type == EBindingType::StorageTexture ||
		type == EBindingType::Sampler;
}


VulkanDescriptorSetMgr::VulkanDescriptorSetMgr(VulkanDevice* device) : m_Device(device), m_ReservePool(VK_NULL_HANDLE), m_PoolMaxIndex(VK_INVALID_INDEX) {
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
	if(m_ReservePool) {
		vkDestroyDescriptorPool(m_Device->GetDevice(), m_ReservePool, nullptr);
	}
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
	return m_Pools[m_PoolMaxIndex];
}

VkDescriptorPool VulkanDescriptorSetMgr::GetReservePool() {
	if(VK_NULL_HANDLE == m_ReservePool) {
		CreatePool(&m_ReservePool);
	}
	return m_ReservePool;
}

VkDescriptorSet VulkanDescriptorSetMgr::AllocateDescriptorSet(VkDescriptorSetLayout layout) {
	VkDescriptorSet set;
	AllocateDescriptorSets({ layout }, { set });
	return set;
}

bool VulkanDescriptorSetMgr::AllocateDescriptorSets(TConstArrayView<VkDescriptorSetLayout> layouts, TArrayView<VkDescriptorSet> sets) {
	VkDescriptorSetAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, nullptr };
	allocateInfo.descriptorPool = GetPool();
	allocateInfo.descriptorSetCount = layouts.Size();
	allocateInfo.pSetLayouts = layouts.Data();
	VkResult allocResult = vkAllocateDescriptorSets(m_Device->GetDevice(), &allocateInfo, sets.Data());
	switch (allocResult) {
	case VK_SUCCESS:
		//all good, return
		return true;
	case VK_ERROR_FRAGMENTED_POOL:
	case VK_ERROR_OUT_OF_POOL_MEMORY:
		//reallocate pool
		break;
	default:
		LOG_WARNING("[VulkanDescriptorSetMgr::AllocateDescriptorSets] Could not allocate descriptor sets!");
		//unrecoverable error
		return false;
	}
	
	AddPool();
	return AllocateDescriptorSets(layouts, sets);
}

void VulkanDescriptorSetMgr::BeginFrame() {
	// reset pools every frame
	m_PoolMaxIndex = 0;
	for(auto pool: m_Pools) {
		vkResetDescriptorPool(m_Device->GetDevice(), pool, 0);
	}
}

void VulkanDescriptorSetMgr::CreatePool(VkDescriptorPool* poolPtr) {
	VkDescriptorPoolCreateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, nullptr };
	info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	constexpr uint8 bindingCount = static_cast<uint8>(EBindingType::MaxNum);
	static const uint32 s_CountPerType[bindingCount] = { 32, 128, 64, 128, 64 };
	VkDescriptorPoolSize sizes[bindingCount];
	uint32 allCount = 0;
	for (uint8 i = 0; i < static_cast<uint8>(EBindingType::MaxNum); ++i) {
		sizes[i].type = ToVkDescriptorType(static_cast<EBindingType>(i));
		sizes[i].descriptorCount = s_CountPerType[i];
		allCount += sizes[i].descriptorCount;
	}
	info.poolSizeCount = bindingCount;
	info.pPoolSizes = sizes;
	info.maxSets = allCount;
	VK_CHECK(vkCreateDescriptorPool(m_Device->GetDevice(), &info, nullptr, poolPtr));
}

void VulkanDescriptorSetMgr::AddPool() {
	m_PoolMaxIndex = (VK_INVALID_INDEX == m_PoolMaxIndex) ? 0 : (m_PoolMaxIndex + 1);
	if(m_PoolMaxIndex < m_Pools.Size()) {
		return;
	}
	CreatePool(&m_Pools.EmplaceBack());
	if(m_Pools.Size() > 1) {
		LOG_WARNING("[VulkanDescriptorSetMgr::AddPool] Pool Size = %u", m_Pools.Size());
	}
}

void VulkanPipelineLayout::Build(VulkanDevice* device, TConstArrayView<RHIShaderParamSetLayout> meta) {
	CHECK(VK_NULL_HANDLE == Handle);
	const uint32 layoutCount = meta.Size();
	DescriptorSetLayouts.Resize(layoutCount);
	for (uint32 i = 0; i < layoutCount; ++i) {
		DescriptorSetLayouts[i] = device->GetDescriptorMgr()->GetLayoutHandle(meta[i]);
	}
	VkPipelineLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0 };
	layoutInfo.setLayoutCount = DescriptorSetLayouts.Size();
	layoutInfo.pSetLayouts = DescriptorSetLayouts.Data();
	VK_ASSERT(vkCreatePipelineLayout(device->GetDevice(), &layoutInfo, nullptr, &Handle), "PipelineLayout");
	PipelineLayoutMeta = meta;
}

VulkanRHIGraphicsPipelineState::VulkanRHIGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc, VulkanDevice* device) :RHIGraphicsPipelineState(desc), m_Device(device) {
	// Create pipeline layout
	{
		m_PipelineLayout.Build(m_Device, m_Desc.Layout);
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
		colorBlendInfo.logicOpEnable = blendDesc.LogicOpEnable;
		colorBlendInfo.logicOp = ToVkLogicOp(blendDesc.LogicOp);
		uint32 blendStateCount = desc.NumColorTargets;
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
		uint32 colorAttachmentCount = desc.NumColorTargets;
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
		pipelineInfo.layout = m_PipelineLayout.Handle;

		VK_ASSERT(vkCreateGraphicsPipelines(m_Device->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline), "vkCreateGraphicsPipelines");
	}
}

VulkanRHIGraphicsPipelineState::~VulkanRHIGraphicsPipelineState() {
	vkDestroyPipelineLayout(m_Device->GetDevice(), m_PipelineLayout.Handle, nullptr);
	vkDestroyPipeline(m_Device->GetDevice(), m_Pipeline, nullptr);
}

void VulkanRHIGraphicsPipelineState::SetName(const char* name) {
	VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_PIPELINE, m_Pipeline, name);
}

VulkanRHIComputePipelineState::VulkanRHIComputePipelineState(const RHIComputePipelineStateDesc& desc, VulkanDevice* device) : RHIComputePipelineState(desc), m_Device(device) {
	// Create pipeline layout
	{
		m_PipelineLayout.Build(m_Device, m_Desc.Layout);
	}

	// create pipeline
	{
		VkComputePipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO, nullptr, 0 };
		pipelineInfo.stage = static_cast<VulkanRHIShader*>(m_Desc.Shader)->GetShaderStageInfo();
		pipelineInfo.layout = m_PipelineLayout.Handle;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = 0;
		vkCreateComputePipelines(m_Device->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline);
	}
}

VulkanRHIComputePipelineState::~VulkanRHIComputePipelineState() {
	vkDestroyPipelineLayout(m_Device->GetDevice(), m_PipelineLayout.Handle, nullptr);
	vkDestroyPipeline(m_Device->GetDevice(), m_Pipeline, nullptr);
}

void VulkanRHIComputePipelineState::SetName(const char* name) {
	VK_SET_OBJECT_NAME(VK_OBJECT_TYPE_PIPELINE, m_Pipeline, name);
}

VulkanDescriptorSetParamCache::VulkanDescriptorSetParamCache(const RHIShaderParamSetLayout& layout) : m_LayoutRef(layout) {
	// compile layout
	uint32 bufferIndex = 0, imageIndex = 0;
	for (auto& binding : m_LayoutRef) {
		if (BindingIsBuffer(binding.Type)) {
			bufferIndex += binding.Count;
		}
		else if (BindingIsImage(binding.Type)) {
			imageIndex += binding.Count;
		}
	}
	m_WriteBuffers.Resize(bufferIndex, {});
	m_WriteImages.Resize(imageIndex, {});
	m_Writes.Resize(m_LayoutRef.Size(), {});
	// allocate descriptor writes
	bufferIndex = 0;
	imageIndex = 0;
	for (uint32 i = 0; i < m_LayoutRef.Size(); ++i) {
		auto& src = m_LayoutRef[i];
		auto& write = m_Writes[i];
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.pNext = nullptr;
		write.dstSet = VK_NULL_HANDLE;// update by outer objects
		write.dstBinding = i;
		write.dstArrayElement = 0;
		write.descriptorCount = src.Count;
		write.descriptorType = ToVkDescriptorType(src.Type);
		if (BindingIsBuffer(src.Type)) {
			write.pBufferInfo = &m_WriteBuffers[bufferIndex];
			bufferIndex += src.Count;
		}
		else if (BindingIsImage(src.Type)) {
			write.pImageInfo = &m_WriteImages[imageIndex];
			imageIndex += src.Count;
		}
	}
}

bool VulkanDescriptorSetParamCache::SetParam(const VulkanDynamicBufferAllocator* allocator, uint32 bindIndex, const RHIShaderParam& param) {
	CHECK(m_LayoutRef[bindIndex].Type == param.Type);
	auto& write = m_Writes[bindIndex];
	switch (param.Type) {
	case EBindingType::UniformBuffer:
	case EBindingType::StorageBuffer: {
		auto& bufferInfo = const_cast<VkDescriptorBufferInfo*>(write.pBufferInfo)[param.ArrayIndex];
		VkBuffer bufferHandle;
		uint32 offset, range;
		if (param.IsDynamicBuffer) {
			const RHIDynamicBuffer& dBuffer = param.Data.DynamicBuffer;
			bufferHandle = allocator->GetBufferHandle(param.Data.DynamicBuffer.BufferIndex);
			offset = dBuffer.Offset;
			range = dBuffer.Size;
		}
		else {
			VulkanBufferImpl* buffer = (VulkanBufferImpl*)param.Data.Buffer;
			bufferHandle = buffer->GetBuffer();
			offset = param.Data.Offset;
			range = param.Data.Size;
		}
		if(bufferInfo.buffer == bufferHandle && bufferInfo.offset == offset && bufferInfo.range == range) {
			return false;
		}
		bufferInfo.buffer = bufferHandle;
		bufferInfo.offset = offset;
		bufferInfo.range = range;
	}break;
	case EBindingType::Texture:
	case EBindingType::StorageTexture: {
		VulkanRHITexture* texture = (VulkanRHITexture*)(param.Data.Texture);
		auto& imageInfo = const_cast<VkDescriptorImageInfo*>(write.pImageInfo)[param.ArrayIndex];
		VkImageView targetView = texture->GetView(param.Data.SubRes);
		if(imageInfo.imageView == targetView) {
			return false;
		}
		imageInfo.imageView = texture->GetView(param.Data.SubRes); // TODO more view types
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}break;
	case EBindingType::Sampler: {
		auto& imageInfo = const_cast<VkDescriptorImageInfo*>(write.pImageInfo)[param.ArrayIndex];
		VkSampler targetSampler = ((VulkanRHISampler*)param.Data.Sampler)->GetSampler();
		if(imageInfo.sampler == targetSampler) {
			return false;
		}
		imageInfo.sampler = targetSampler;
	}break;
	default:break;
	}
	return true;
}

VulkanPipelineDescriptorSetCache::VulkanPipelineDescriptorSetCache(VulkanDevice* device):
m_Device(device), m_PipelineData(nullptr), m_PipelineType(EPipelineType::None){}

void VulkanPipelineDescriptorSetCache::BindGraphicsPipeline(const VulkanRHIGraphicsPipelineState* pipeline) {
	m_GraphicsPipeline = pipeline;
	m_PipelineType = EPipelineType::Graphics;
	SetupParameterLayout();
}

void VulkanPipelineDescriptorSetCache::BindComputePipeline(const VulkanRHIComputePipelineState* pipeline) {
	m_ComputePipeline = pipeline;
	m_PipelineType = EPipelineType::Compute;
	SetupParameterLayout();
}

void VulkanPipelineDescriptorSetCache::SetParam(uint32 setIndex, uint32 bindIndex, const RHIShaderParam& param) {
	CHECK(setIndex < m_ParamCaches.Size());
	if(m_ParamCaches[setIndex].SetParam(m_Device->GetDynamicBufferAllocator(), bindIndex, param)) {
		m_DirtySets[setIndex] = true;
	}
}

void VulkanPipelineDescriptorSetCache::Bind(VkCommandBuffer cmd) {	// check if need write
	for (uint32 i = 0; i < m_BindingSets.Size(); ++i) {
		if (m_DirtySets[i]) {
			ReallocateSet(i);
			auto& writes = m_ParamCaches[i].GetWrites();
			for (auto& write : writes) {
				write.dstSet = m_BindingSets[i];
			}
			vkUpdateDescriptorSets(m_Device->GetDevice(), writes.Size(), writes.Data(), 0, nullptr);
			m_DirtySets[i] = false;
		}
	}
	// bind
	vkCmdBindDescriptorSets(cmd, ToVkPipelineBindPoint(m_PipelineType), GetLayout()->Handle, 0, m_BindingSets.Size(), m_BindingSets.Data(), 0, nullptr);
}

void VulkanPipelineDescriptorSetCache::Reset() {
	m_PipelineData = nullptr;
	m_PipelineType = EPipelineType::None;
	m_ParamCaches.Reset();
	m_BindingSets.Reset();
	m_DirtySets.Reset();
	m_AllDescriptorSets.Reset();
}

VkPipelineBindPoint VulkanPipelineDescriptorSetCache::GetVkPipelineBindPoint() {
	return ToVkPipelineBindPoint(m_PipelineType);
}

VkPipeline VulkanPipelineDescriptorSetCache::GetVkPipeline() {
	if(EPipelineType::Graphics == m_PipelineType) {
		return m_GraphicsPipeline->GetPipelineHandle();
	}
	else if (EPipelineType::Compute == m_PipelineType) {
		return m_ComputePipeline->GetPipelineHandle();
	}
	return nullptr;
}

void VulkanPipelineDescriptorSetCache::SetupParameterLayout() {
	m_ParamCaches.Reset();
	if(const VulkanPipelineLayout* layout = GetLayout()) {
		const uint32 setCount = layout->PipelineLayoutMeta.Size();
		// create parameter caches
		m_ParamCaches.Reserve(setCount);
		for (auto& dsLayout : layout->PipelineLayoutMeta) {
			m_ParamCaches.EmplaceBack(dsLayout);
		}
		m_BindingSets.Resize(setCount, VK_NULL_HANDLE); // reserve null
		m_DirtySets.Resize(setCount, true); // mark all ds dirty
	}
}

VkDescriptorSet VulkanPipelineDescriptorSetCache::ReallocateSet(uint32 setIndex) {
	VkDescriptorSet set = m_Device->GetDescriptorMgr()->AllocateDescriptorSet(GetLayout()->DescriptorSetLayouts[setIndex]);
	m_AllDescriptorSets.PushBack(set);
	m_BindingSets[setIndex] = set;
	return set;
}

const VulkanPipelineLayout* VulkanPipelineDescriptorSetCache::GetLayout() {
	if(EPipelineType::Graphics == m_PipelineType) {
		return m_GraphicsPipeline->GetPipelineLayout();
	}
	if (EPipelineType::Compute == m_PipelineType) {
		return m_ComputePipeline->GetPipelineLayout();
	}
	return nullptr;
}
