#pragma once
#include "RHI/Public/RHI.h"
#include "RHI/Public/RHIClasses.h"
#include "RHI/Public/RHIStructs.h"
#include "Core/Public/Container.h"
#include "Core/Public/TPtr.h"
#include "VulkanMemory.h"
#include "VulkanCommon.h"
#include "RHIVKClasses.h"
#include "RHIVKResources.h"

namespace Engine{
	class RHIVulkan final: public RHI {
	private:
		uint8 m_MaxFramesInFlight{ 3 };
		VulkanContext m_Context;
		TUniquePtr<VulkanMemMgr> m_MemMgr;
		TUniquePtr<RHIVulkanSwapChain> m_SwapChain;
		VkCommandPool m_RHICommandPool;
		VkDescriptorPool m_DescriptorPool;

	private:
		void CreateCommandPools();
		void CreateDescriptorPool();
	public:

		explicit RHIVulkan(const RSInitInfo* initInfo);
		~RHIVulkan()override;
		static RHIVulkan* InstanceVulkan();
		static VkDevice GetDevice();

		RSVkImGuiInitInfo GetImGuiInitInfo();
		RHISwapChain* GetSwapChain() override;
		ERHIFormat GetDepthFormat() override;
		RRenderPass* CreateRenderPass(CRefRange<RSAttachment> attachments, CRefRange<RSubPassInfo> subpasses, CRefRange<RSubpassDependency> dependencies) override;
		RRenderPass* CreateRenderPass(CRefRange<RSAttachment> colorAttachments, const RSAttachment* depthAttachment) override;

		void DestroyRenderPass(RRenderPass* pass) override;

		// descriptor set
		RDescriptorSetLayout* CreateDescriptorSetLayout(CRefRange<RSDescriptorSetLayoutBinding> bindings) override;
		void DestroyDescriptorSetLayout(RDescriptorSetLayout* descriptorSetLayout) override;
		RDescriptorSet* AllocateDescriptorSet(const RDescriptorSetLayout* layout) override;
		//void AllocateDescriptorSets(uint32 count, const RDescriptorSetLayout* const* layouts, RDescriptorSet* const* descriptorSets)override;
		//void FreeDescriptorSets(uint32 count, RDescriptorSet** descriptorSets) override;
		void FreeDescriptorSet(RDescriptorSet* descriptorSet) override;

		// pipeline
		RPipelineLayout* CreatePipelineLayout(CRefRange<RDescriptorSetLayout*> layouts, CRefRange<RSPushConstantRange> pushConstants) override;
		void DestroyPipelineLayout(RPipelineLayout* pipelineLayout) override;
		RPipeline* CreateGraphicsPipeline(const RGraphicsPipelineCreateInfo& createInfo, RPipelineLayout* layout, RRenderPass* renderPass, uint32 subpass, RPipeline* basePipeline, int32_t basePipelineIndex) override;
		RPipeline* CreateComputePipeline(const RPipelineShaderInfo& shader, RPipelineLayout* layout, RPipeline* basePipeline, uint32 basePipelineIndex) override;
		void DestroyPipeline(RPipeline* pipeline) override;

		RHICommandBuffer* AllocateCommandBuffer(RCommandBufferLevel level)override;
		void FreeCommandBuffer(RHICommandBuffer* cmd) override;

		void ImmediateSubmit(const CommandBufferFunc& func) override;
		void SubmitCommandBuffer(const RHICommandBuffer* cmd, RHIFence* fence, RHISwapChain* swapChain) override;
		int QueueSubmitPresent(RHICommandBuffer* cmd, uint8 frameIndex, RHIFence* fence) override;
		RHIBuffer* CreateBuffer(const RHIBufferDesc& desc) override;
		RHITexture* CreateTexture(const RHITextureDesc& desc) override;
		RHISampler* CreateSampler(const RHISamplerDesc& desc) override;
		RHIFence* CreateFence(bool sig) override;
		RHIShader* CreateShader(EShaderStageFlagBit type, const char* codeData, size_t codeSize, const char* entryFunc) override;
		RHIGraphicsPipelineState* CreateGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc) override;
	};
}
