#include "RHIVulkan.h"
#include "VulkanUtil.h"
#include "VulkanConverter.h"
#include "VulkanBuilder.h"
#include "Math/Public/MathBase.h"
#include "Resource/Public/Config.h"

namespace Engine {

#define RETURN_RHI_PTR(cls, hd)\
	cls##Vk* cls##Ptr = new cls##Vk;\
	cls##Ptr->handle = hd; \
	return reinterpret_cast<cls*>(cls##Ptr)


#pragma region rhi initialzie
	void RHIVulkan::CreateCommandPools() {
		// graphics command pool
		{
			VkCommandPoolCreateInfo commandPoolCreateInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr};
			commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			commandPoolCreateInfo.queueFamilyIndex = m_Context.GraphicsQueue.FamilyIndex;
			VK_CHECK(vkCreateCommandPool(GetDevice(), &commandPoolCreateInfo, nullptr, &m_RHICommandPool), "VkCreateCommandPool");
		}
	}

	void RHIVulkan::CreateDescriptorPool() {
		// Since DescriptorSet should be treated as asset in Vulkan, DescriptorPool
		// should be big enough, and thus we can sub-allocate DescriptorSet from
		// DescriptorPool merely as we sub-allocate Buffer/Image from DeviceMemory.

		uint32 maxVertexBlendingMeshCount{ 256 };
		uint32 maxMaterialCount{ 256 };

		uint32 poolCount = 7;
		TempArray<VkDescriptorPoolSize> poolSizes(poolCount);
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
		poolSizes[0].descriptorCount = 3 + 2 + 2 + 2 + 1 + 1 + 3 + 3;
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSizes[1].descriptorCount = 1 + 1 + 1 * maxVertexBlendingMeshCount;
		poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[2].descriptorCount = 1 * maxMaterialCount;
		poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[3].descriptorCount = 3 + 5 * maxMaterialCount + 1 + 1; // ImGui_ImplVulkan_CreateDeviceObjects
		poolSizes[4].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		poolSizes[4].descriptorCount = 4 + 1 + 1 + 2;
		poolSizes[5].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		poolSizes[5].descriptorCount = 3;
		poolSizes[6].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		poolSizes[6].descriptorCount = 1;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = poolCount;
		poolInfo.pPoolSizes = poolSizes.Data();
		poolInfo.maxSets = 1 + 1 + 1 + maxMaterialCount + maxVertexBlendingMeshCount + 1 + 1; // +skybox + axis descriptor set
		poolInfo.flags = 0U;
		VK_CHECK(vkCreateDescriptorPool(GetDevice(), &poolInfo, nullptr, &m_DescriptorPool), "VkCreateDescriptorPool");
	}

	RHIVulkan::RHIVulkan(const RSInitInfo* initInfo) {
		m_MaxFramesInFlight = initInfo->MaxFramesInFlight;

		// initialize context
		ContextBuilder initializer(m_Context);
		initializer.AppName = initInfo->ApplicationName;
		initializer.WindowHandle = initInfo->WindowHandle;
		if (GetConfig().GPUType == GPU_INTEGRATED) {
			initializer.Flags |= ContextBuilder::INTEGRATED_GPU;
		}
		if (initInfo->EnableDebug) {
			initializer.Flags |= ContextBuilder::ENABLE_DEBUG;
		}
		initializer.Build();

		// create swap chain
		m_SwapChain.reset(new RHIVulkanSwapChain(&m_Context));

		// create memory manager
		m_MemMgr.reset(new VulkanMemMgr(&m_Context));

		CreateCommandPools();
		CreateDescriptorPool();
		LOG("RHI: Vulkan initialized successfully!");
	}

	RHIVulkan::~RHIVulkan() {
		//vma
		m_MemMgr.reset();
		m_SwapChain.reset();
		vkDestroyCommandPool(GetDevice(), m_RHICommandPool, nullptr);
		vkDestroyDescriptorPool(GetDevice(), m_DescriptorPool, nullptr);
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
			return FORMAT_D32_SFLOAT;
		case(VK_FORMAT_D24_UNORM_S8_UINT):
			return FORMAT_D24_UNORM_S8_UINT;
		case(VK_FORMAT_D16_UNORM_S8_UINT):
			return FORMAT_D16_UNORM;
		default:
			return FORMAT_UNDEFINED;
		}
	}

#pragma endregion

	VkDevice RHIVulkan::GetDevice() {
		return InstanceVulkan()->m_Context.Device;
	}

	RHIVulkan* RHIVulkan::InstanceVulkan() {
		return dynamic_cast<RHIVulkan*>(Instance());
	}

	RRenderPass* RHIVulkan::CreateRenderPass(CRefRange<RSAttachment> attachments, CRefRange<RSubPassInfo> subpasses, CRefRange<RSubpassDependency> dependencies) {
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

	RRenderPass* RHIVulkan::CreateRenderPass(CRefRange<RSAttachment> colorAttachments, const RSAttachment* depthAttachment) {
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
	RDescriptorSetLayout* RHIVulkan::CreateDescriptorSetLayout(CRefRange<RSDescriptorSetLayoutBinding> bindings) {
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

	/*
	void RHIVulkan::AllocateDescriptorSets(uint32 count, const RDescriptorSetLayout* const* layouts, RDescriptorSet* const* descriptorSets) {
		TArray<VkDescriptorSetLayout> layoutsVk(count);
		for(uint32 i=0; i< count; ++i) {
			layoutsVk[i] = ((RDescriptorSetLayoutVk*)layouts[i])->handle;
		}
		VkDescriptorSetAllocateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
		info.pNext = nullptr;
		info.descriptorPool = m_DescriptorPool;
		info.descriptorSetCount = count;
		info.pSetLayouts = layoutsVk.Data();
		TArray<VkDescriptorSet> descriptorSetsVk(count);
		if(VK_SUCCESS != vkAllocateDescriptorSets(GetDevice(), &info, descriptorSetsVk.Data())) {
			return;
		}
		for(uint32 i=0; i< count; ++i) {
			RDescriptorSetVk* descriptorSetVk = (RDescriptorSetVk*)descriptorSets[i];
			descriptorSetVk->handle = descriptorSetsVk[i];
		}
	}
	void RHIVulkan::FreeDescriptorSets(uint32 count, RDescriptorSet** descriptorSets) {
		TArray<VkDescriptorSet> descriptorSetsVk(count);
		for(uint32 i=0; i<count; ++i) {
			descriptorSetsVk[i] = ((RDescriptorSetVk*)descriptorSets[i])->handle;
			delete descriptorSets[i];
		}
		vkFreeDescriptorSets(GetDevice(), m_DescriptorPool, count, descriptorSetsVk.Data());
	}
	*/
	
	RPipelineLayout* RHIVulkan::CreatePipelineLayout(CRefRange<RDescriptorSetLayout*> layouts, CRefRange<RSPushConstantRange> pushConstants) {
		uint32 i;
		VkPipelineLayoutCreateInfo info{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		info.flags = 0;
		info.pNext = nullptr;
		info.setLayoutCount = layouts.Size();
		TempArray<VkDescriptorSetLayout> setLayoutsVk(layouts.Size());
		for (i = 0; i < layouts.Size(); ++i) {
			setLayoutsVk[i] = ((RDescriptorSetLayoutVk*)layouts[i])->handle;
		}
		info.pSetLayouts = setLayoutsVk.Data();
		
		info.pushConstantRangeCount = pushConstants.Size();
		if(pushConstants.Size() > 0) {
			TempArray<VkPushConstantRange> pushConstantRangesVk(pushConstants.Size());
			for(i=0; i< pushConstants.Size(); ++i) {
				pushConstantRangesVk[i].offset = pushConstants[i].offset;
				pushConstantRangesVk[i].size = pushConstants[i].size;
				pushConstantRangesVk[i].stageFlags = pushConstants[i].stageFlags;
			}
			info.pPushConstantRanges = pushConstantRangesVk.Data();
		}

		VkPipelineLayout handle;
		if(VK_SUCCESS != vkCreatePipelineLayout(GetDevice(), &info, nullptr, &handle)) {
			return nullptr;
		}
		RPipelineLayoutVk* pipelineLayout = new RPipelineLayoutVk;
		pipelineLayout->handle = handle;
		return pipelineLayout;
	}

	void RHIVulkan::DestroyPipelineLayout(RPipelineLayout* pipelineLayout) {
		RPipelineLayoutVk* pipelineLayoutVk = (RPipelineLayoutVk*)pipelineLayout;
		vkDestroyPipelineLayout(GetDevice(), pipelineLayoutVk->handle, nullptr);
		delete pipelineLayoutVk;
	}

	RPipeline* RHIVulkan::CreateGraphicsPipeline(const RGraphicsPipelineCreateInfo& info, RPipelineLayout* layout, RRenderPass* renderPass, uint32 subpass, RPipeline* basePipeline, int32_t basePipelineIndex) {
		uint32 i; // iter
		// shader stages
		TempArray<VkShaderModuleCreateInfo> shaderModuleInfos(info.Shaders.Size());
		TempArray<VkShaderModule> shaderModules(info.Shaders.Size());
		TempArray<VkPipelineShaderStageCreateInfo> shaderInfos(info.Shaders.Size());
		for (i = 0; i < info.Shaders.Size(); ++i) {
			shaderModuleInfos[i] = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr, 0 };
			shaderModuleInfos[i].codeSize = info.Shaders[i].code.Size();
			shaderModuleInfos[i].pCode = reinterpret_cast<const uint32*>(info.Shaders[i].code.Data());
			vkCreateShaderModule(GetDevice(), &shaderModuleInfos[i], nullptr, &shaderModules[i]);

			shaderInfos[i] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0 };
			shaderInfos[i].module = shaderModules[i];
			shaderInfos[i].pName = info.Shaders[i].funcName;
			shaderInfos[i].stage = (VkShaderStageFlagBits)info.Shaders[i].stage;
		}

		// vertex input
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, nullptr, 0 };
		TempArray<VkVertexInputBindingDescription> vertexInputBindings(info.Bindings.Size());
		for(i=0; i< info.Bindings.Size(); ++i) {
			vertexInputBindings[i].binding = info.Bindings[i].binding;
			vertexInputBindings[i].stride = info.Bindings[i].stride;
			vertexInputBindings[i].inputRate = (VkVertexInputRate)info.Bindings[i].inputRate;
		}
		vertexInputInfo.vertexBindingDescriptionCount = info.Bindings.Size();
		vertexInputInfo.pVertexBindingDescriptions = vertexInputBindings.Data();
		TempArray<VkVertexInputAttributeDescription> vertexInputAttrs(info.Attributes.Size());
		for(i=0; i< info.Attributes.Size(); ++i) {
			vertexInputAttrs[i].location = i;
			vertexInputAttrs[i].binding = info.Attributes[i].binding;
			vertexInputAttrs[i].format = ToVkFormat(info.Attributes[i].format);
			vertexInputAttrs[i].offset = info.Attributes[i].offset;
		}
		vertexInputInfo.vertexAttributeDescriptionCount = info.Attributes.Size();
		vertexInputInfo.pVertexAttributeDescriptions = vertexInputAttrs.Data();

		// input assembly
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, nullptr, 0 };
		inputAssemblyInfo.primitiveRestartEnable = info.PrimitiveRestartEnable;
		inputAssemblyInfo.topology = (VkPrimitiveTopology)info.Topology;

		// tessellation
		VkPipelineTessellationStateCreateInfo tessellationInfo{ VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO, nullptr, 0 };
		tessellationInfo.patchControlPoints = info.PatchControlPoints;

		// viewport
		VkPipelineViewportStateCreateInfo viewportInfo{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, nullptr, 0 };
		auto& viewport = info.Viewport;
		VkViewport viewportVk{ viewport.x, viewport.y, viewport.width, viewport.height, viewport.minDepth, viewport.maxDepth };
		viewportInfo.viewportCount = 1;
		viewportInfo.pViewports = &viewportVk;
		auto& scissor = info.Scissor;
		VkRect2D scissorVk{ {(int32)scissor.x, (int32)scissor.y}, {scissor.w, scissor.h} };
		viewportInfo.scissorCount = 1;
		viewportInfo.pScissors = &scissorVk;

		// rasterization
		VkPipelineRasterizationStateCreateInfo rasterizationInfo = TranslateVkPipelineRasterizationState(info);
		// multi sample
		VkPipelineMultisampleStateCreateInfo multisampleInfo = TranslatePipelineMultisample(info);
		// depth stencil
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo = TranslatePipelineDepthStencil(info);

		// color blend
		TempArray<VkPipelineColorBlendAttachmentState> attachments(info.AttachmentStates.Size());
		for(i=0; i<info.AttachmentStates.Size(); ++i) {
			TranslateColorBlendAttachmentState(attachments[i], info.AttachmentStates[i]);
		}
		VkPipelineColorBlendStateCreateInfo colorBlendInfo{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, nullptr, 0 };
		colorBlendInfo.logicOpEnable = info.LogicOpEnable;
		colorBlendInfo.logicOp = (VkLogicOp)info.LogicOp;
		colorBlendInfo.attachmentCount = info.AttachmentStates.Size();
		colorBlendInfo.pAttachments = attachments.Data();
		colorBlendInfo.blendConstants[0] = info.BlendConstants[0];
		colorBlendInfo.blendConstants[1] = info.BlendConstants[1];
		colorBlendInfo.blendConstants[2] = info.BlendConstants[2];
		colorBlendInfo.blendConstants[3] = info.BlendConstants[3];

		// dynamic
		VkPipelineDynamicStateCreateInfo dynamicInfo{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, nullptr, 0 };
		dynamicInfo.dynamicStateCount = info.DynamicStates.Size();
		dynamicInfo.pDynamicStates = (const VkDynamicState*)info.DynamicStates.Data();

		// pipeline
		VkGraphicsPipelineCreateInfo createInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, nullptr, 0 };
		createInfo.stageCount = info.Shaders.Size();
		createInfo.pStages = shaderInfos.Data();
		createInfo.pVertexInputState = &vertexInputInfo;
		createInfo.pInputAssemblyState = &inputAssemblyInfo;
		createInfo.pTessellationState = &tessellationInfo;
		createInfo.pViewportState = &viewportInfo;
		createInfo.pRasterizationState = &rasterizationInfo;
		createInfo.pMultisampleState = &multisampleInfo;
		createInfo.pDepthStencilState = &depthStencilInfo;
		createInfo.pColorBlendState = &colorBlendInfo;
		createInfo.pDynamicState = &dynamicInfo;
		createInfo.layout = ((RPipelineLayoutVk*)layout)->handle;
		createInfo.renderPass = ((RRenderPassVk*)renderPass)->handle;
		createInfo.subpass = subpass;
		createInfo.basePipelineHandle = nullptr == basePipeline ? VK_NULL_HANDLE : ((RPipelineVk*)basePipeline)->handle;
		createInfo.basePipelineIndex = basePipelineIndex;

		VkPipeline handle;
		VkResult res = vkCreateGraphicsPipelines(GetDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &handle);
		for(i=0; i< info.Shaders.Size(); ++i) {
			vkDestroyShaderModule(GetDevice(), shaderModules[i], nullptr);
		}
		if(VK_SUCCESS != res) {
			return nullptr;
		}
		RPipelineVk* pipeline = new RPipelineVk;
		pipeline->handle = handle;
		pipeline->m_Type = PIPELINE_GRAPHICS;
		return pipeline;
	}

	RPipeline* RHIVulkan::CreateComputePipeline(const RPipelineShaderInfo& shader, RPipelineLayout* layout, RPipeline* basePipeline, uint32 basePipelineIndex) {
		VkComputePipelineCreateInfo createInfo{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO, nullptr, 0 };
		// shader stages
		VkShaderModuleCreateInfo shaderModuleInfo;
		VkShaderModule shaderModule;
		VkPipelineShaderStageCreateInfo& shaderInfo = createInfo.stage;
		shaderModuleInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr, 0 };
		shaderModuleInfo.codeSize = shader.code.Size();
		shaderModuleInfo.pCode = reinterpret_cast<const uint32*>(shader.code.Data());
		vkCreateShaderModule(GetDevice(), &shaderModuleInfo, nullptr, &shaderModule);

		shaderInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0 };
		shaderInfo.module = shaderModule;
		shaderInfo.pName = shader.funcName;
		shaderInfo.stage = (VkShaderStageFlagBits)shader.stage;

		createInfo.layout = ((RPipelineLayoutVk*)layout)->handle;
		createInfo.basePipelineHandle = nullptr == basePipeline ? VK_NULL_HANDLE : ((RPipelineVk*)basePipeline)->handle;
		createInfo.basePipelineIndex = basePipelineIndex;

		VkPipeline handle;
		VkResult res = vkCreateComputePipelines(GetDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &handle);
		vkDestroyShaderModule(GetDevice(), shaderModule, nullptr);
		if(VK_SUCCESS != res) {
			return nullptr;
		}
		RPipelineVk* pipeline = new RPipelineVk;
		pipeline->handle = handle;
		pipeline->m_Type = PIPELINE_COMPUTE;
		return pipeline;
	}

	void RHIVulkan::DestroyPipeline(RPipeline* pipeline) {
		RPipelineVk* pipelineVk = (RPipelineVk*)pipeline;
		vkDestroyPipeline(GetDevice(), pipelineVk->handle, nullptr);
		delete pipelineVk;
	}

	/*
	void RHIVulkan::QueueSubmit(CRefRange<RHICommandBuffer*> cmds, CRefRange<RSemaphore*> waitSemaphores, CRefRange<RPipelineStageFlags> waitStageFlags, CRefRange<RSemaphore*> signalSemaphores, RHIFence* fence) {
		VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.pNext = nullptr;
		uint32 i;

		// wait semaphores
		TempArray<VkSemaphore> vkWaitSmps(waitSemaphores.Size());
		for (i = 0; i < waitSemaphores.Size(); ++i) {
			vkWaitSmps[i] = dynamic_cast<RSemaphoreVk*>(waitSemaphores[i])->handle;
		}
		submitInfo.waitSemaphoreCount = waitSemaphores.Size();
		submitInfo.pWaitSemaphores = vkWaitSmps.Data();
		submitInfo.pWaitDstStageMask = (VkPipelineStageFlags*)waitStageFlags.Data();

		// commands
		TempArray<VkCommandBuffer> vkCmds(cmds.Size());
		for (i = 0; i < cmds.Size(); ++i) {
			vkCmds[i] = dynamic_cast<RCommandBufferVk*>(cmds[i])->handle;
		}
		submitInfo.commandBufferCount = cmds.Size();
		submitInfo.pCommandBuffers = vkCmds.Data();

		// signal semaphores
		TempArray<VkSemaphore> vkSignalSmps(signalSemaphores.Size());
		for(i=0; i< signalSemaphores.Size(); ++i) {
			vkSignalSmps[i] = dynamic_cast<RSemaphoreVk*>(signalSemaphores[i])->handle;
		}
		submitInfo.signalSemaphoreCount = signalSemaphores.Size();
		submitInfo.pSignalSemaphores = vkSignalSmps.Data();

		// fence
		VkFence vkFence = (nullptr == fence) ? VK_NULL_HANDLE : dynamic_cast<RHIVkFence*>(fence)->handle;

		VK_CHECK(vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, vkFence), "vkQueueSubmit");
	}
	*/
	RHICommandBuffer* RHIVulkan::AllocateCommandBuffer(RCommandBufferLevel level) {
		VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocateInfo.pNext = nullptr;
		allocateInfo.commandPool = m_RHICommandPool;
		allocateInfo.level = (VkCommandBufferLevel)level;
		allocateInfo.commandBufferCount = 1;
		VkCommandBuffer handle;
		if(VK_SUCCESS != vkAllocateCommandBuffers(GetDevice(), &allocateInfo, &handle)){
			return nullptr;
		}
		RHIVulkanCommandBuffer* cmd = new RHIVulkanCommandBuffer;
		cmd->handle = handle;
		cmd->m_Pool = allocateInfo.commandPool;
		return cmd;
	}

	void RHIVulkan::FreeCommandBuffer(RHICommandBuffer* cmd) {
		RHIVulkanCommandBuffer* vkCmd = dynamic_cast<RHIVulkanCommandBuffer*>(cmd);
		vkFreeCommandBuffers(GetDevice(), vkCmd->m_Pool, 1, &vkCmd->handle);
		delete vkCmd;
	}

	void RHIVulkan::ImmediateSubmit(const CommandBufferFunc& func) {
		RHIVulkanCommandBuffer cmd;

		// allocate
		VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocateInfo.pNext = nullptr;
		allocateInfo.commandPool = m_RHICommandPool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = 1;
		VK_CHECK(vkAllocateCommandBuffers(GetDevice(), &allocateInfo, &cmd.handle), "Failed to allocate command buffers!");
		cmd.m_Pool = m_RHICommandPool;

		// begin
		VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		beginInfo.pNext = nullptr;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		beginInfo.pInheritanceInfo = nullptr;
		VK_CHECK(vkBeginCommandBuffer(cmd.handle, &beginInfo), "Failed to begin command buffer!");

		// run
		func(&cmd);

		// end
		vkEndCommandBuffer(cmd.handle);

		// submit
		VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmd.handle;
		vkQueueSubmit(m_Context.GraphicsQueue.Handle, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(m_Context.GraphicsQueue.Handle);

		// free
		vkFreeCommandBuffers(GetDevice(), cmd.m_Pool, 1, &cmd.handle);
	}

	void RHIVulkan::SubmitCommandBuffer(const RHICommandBuffer* cmd, RHIFence* fence, RHISwapChain* swapChain) {
	}

	int RHIVulkan::QueueSubmitPresent(RHICommandBuffer* cmd, uint8 frameIndex, RHIFence* fence) {
		// submit command buffer
		//VkSemaphore semaphores[2] = { m_ImageAvaliableForTextureCopySemaphores[m_CurrentFrame], m_PresentationFinishSemaphores[m_CurrentFrame] };
		VkSemaphore semaphores[1] = { m_PresentationFinishSemaphores[frameIndex] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		VkCommandBuffer vkCmd = dynamic_cast<RHIVulkanCommandBuffer*>(cmd)->handle;
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

} // namespace Engine
