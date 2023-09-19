#include "RHIVKResources.h"
#include "VulkanUtil.h"
#include "RHIVulkan.h"
#include "VulkanBuilder.h"
#include "VulkanLayout.h"
namespace Engine {
#define SET_VK_OBJECT_NAME(type, handle, name) do{\
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
		SET_VK_OBJECT_NAME(VK_OBJECT_TYPE_BUFFER, m_Buffer, name);
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
		SET_VK_OBJECT_NAME(VK_OBJECT_TYPE_IMAGE, m_Image, name);
		SET_VK_OBJECT_NAME(VK_OBJECT_TYPE_IMAGE_VIEW, m_View, name);
    }

    RHIVkTextureWithMem::~RHIVkTextureWithMem() {
		m_Mem.Free();
    }

    RHIVkSampler::~RHIVkSampler() {
		VkDevice device = RHIVulkan::GetDevice();
		vkDestroySampler(device, m_Sampler, nullptr);
	}

	void RHIVkSampler::SetName(const char* name) {
		SET_VK_OBJECT_NAME(VK_OBJECT_TYPE_SAMPLER, m_Sampler, name);
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
		SET_VK_OBJECT_NAME(VK_OBJECT_TYPE_FENCE, m_Handle, name);
	}

	void RHIVulkanSwapChain::Resize(USize2D size) {
		ClearSwapChain();
		CreateSwapChain();
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

	VkShaderStageFlagBits RHIVulkanShader::GetVkStage() const {
		return ToVkShaderStageFlagBit(m_Type);
	}

	void RHIVulkanShader::SetName(const char* name) {
		SET_VK_OBJECT_NAME(VK_OBJECT_TYPE_SHADER_MODULE, m_ShaderModule, name);
	}

	void RHIVulkanShader::GetPipelineShaderCreateInfo(VkPipelineShaderStageCreateInfo& info) const {
		info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		info.pNext = nullptr;
		info.flags = 0;
		info.stage = ToVkShaderStageFlagBit(m_Type);
		info.module = m_ShaderModule;
		info.pName = m_EntryName.data();
		info.pSpecializationInfo = nullptr;
	}

	RHIVulkanGraphicsPipelineState::RHIVulkanGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc):RHIGraphicsPipelineState(desc) {
		VkDevice device = RHIVulkan::GetDevice();
		// create layout
		uint32 layoutCount = desc.Layout.Size();
		TempArray<VkDescriptorSetLayout> layouts(layoutCount);
		for(uint32 i=0; i<layoutCount; ++i) {
			layouts[i] = VulkanLayoutMgr::Instance()->GetLayoutHandle(desc.Layout[i]);
		}
		VkPipelineLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0 };
		layoutInfo.setLayoutCount = layoutCount;
		layoutInfo.pSetLayouts = layouts.Data();
		vkCreatePipelineLayout(device, &layoutInfo, nullptr, &m_PipelineLayout);
	}

	RHIVulkanGraphicsPipelineState::~RHIVulkanGraphicsPipelineState() {
		VkDevice device = RHIVulkan::GetDevice();
		vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
		vkDestroyPipeline(device, m_Pipeline, nullptr);
	}

	void RHIVulkanGraphicsPipelineState::SetName(const char* name) {
		m_Name = name;
	}

	void RHIVulkanGraphicsPipelineState::CreatePipelineHandle(VkRenderPass pass, uint32 subPass) {
		VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, nullptr, 0 };
		// shader stage

		auto& shaders = m_Desc.Shaders;
		uint32 shaderCount = shaders.Size();
		TempArray<VkPipelineShaderStageCreateInfo> shaderStages(shaderCount);
		for(uint32 i=0; i<shaderCount; ++i) {
			RHIVulkanShader* vkShader = dynamic_cast<RHIVulkanShader*>(shaders[i]);
			shaderStages[i] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
				ToVkShaderStageFlagBit(vkShader->GetStage()),
				vkShader->GetShaderModule(),
				vkShader->GetEntry().c_str(),
				nullptr,
			};
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
}
