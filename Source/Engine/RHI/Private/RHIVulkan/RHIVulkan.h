#pragma once
#include "RHI/Public/RHI.h"
#include "Core/Public/Container.h"
#include "Core/Public/TPtr.h"
#include "VulkanCommon.h"
#include "RHIVKResources.h"
#include "VulkanMemory.h"
#include "VulkanCommand.h"
#include "VulkanDesc.h"

class RHIVulkan final: public RHI {
public:

	explicit RHIVulkan(const RHIInitDesc& desc);
	~RHIVulkan()override;
	static RHIVulkan* InstanceVulkan();
	static VkDevice GetDevice();

	RSVkImGuiInitInfo GetImGuiInitInfo();
	RHISwapChain* GetSwapChain() override;
	ERHIFormat GetDepthFormat() override;
	RRenderPass* CreateRenderPass(TConstArrayView<RSAttachment> attachments, TConstArrayView<RSubPassInfo> subpasses, TConstArrayView<RSubpassDependency> dependencies) override;
	RRenderPass* CreateRenderPass(TConstArrayView<RSAttachment> colorAttachments, const RSAttachment* depthAttachment) override;

	void DestroyRenderPass(RRenderPass* pass) override;

	// descriptor set
	RDescriptorSetLayout* CreateDescriptorSetLayout(TConstArrayView<RSDescriptorSetLayoutBinding> bindings) override;
	void DestroyDescriptorSetLayout(RDescriptorSetLayout* descriptorSetLayout) override;
	RDescriptorSet* AllocateDescriptorSet(const RDescriptorSetLayout* layout) override;
	//void AllocateDescriptorSets(uint32 count, const RDescriptorSetLayout* const* layouts, RDescriptorSet* const* descriptorSets)override;
	//void FreeDescriptorSets(uint32 count, RDescriptorSet** descriptorSets) override;
	void FreeDescriptorSet(RDescriptorSet* descriptorSet) override;

	void ImmediateSubmit(const CommandBufferFunc& func) override;
	void SubmitCommandBuffer(const RHICommandBuffer* cmd, RHIFence* fence, RHISwapChain* swapChain) override;
	int QueueSubmitPresent(RHICommandBuffer* cmd, uint8 frameIndex, RHIFence* fence) override;

	RHIBuffer* CreateBuffer(const RHIBufferDesc& desc) override;
	RHITexture* CreateTexture(const RHITextureDesc& desc) override;
	RHISampler* CreateSampler(const RHISamplerDesc& desc) override;
	RHIFence* CreateFence(bool sig) override;
	RHIShader* CreateShader(EShaderStageFlagBit type, const char* codeData, size_t codeSize, const char* entryFunc) override;
	RHIGraphicsPipelineState* CreateGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc) override;
	RHIComputePipelineState* CreateComputePipelineState(const RHIComputePipelineStateDesc& desc) override;
	RHIRenderPass* CreateRenderPass(const RHIRenderPassDesc& desc) override;
	RHICommandBuffer* CreateCommandBuffer()override;
private:
	uint8 m_MaxFramesInFlight{ 3 };
	VulkanContext m_Context;
	TUniquePtr<VulkanMemMgr> m_MemMgr;
	TUniquePtr<RHIVulkanSwapChain> m_SwapChain;
	TUniquePtr<VulkanCommandMgr> m_CmdMgr;
	TUniquePtr<VulkanDSMgr> m_DSMgr;
	VkCommandPool m_RHICommandPool;

private:
	void CreateCommandPools();
};
