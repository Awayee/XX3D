#pragma once
#include "RHI/Public/RHI.h"
#include "Core/Public/TUniquePtr.h"
#include "VulkanCommon.h"
#include "VulkanResources.h"
#include "VulkanMemory.h"
#include "VulkanCommand.h"
#include "VulkanDescriptorSet.h"
#include "VulkanSwapchain.h"

class RHIVulkan final: public RHI {
public:

	explicit RHIVulkan(const RHIInitDesc& desc);
	~RHIVulkan()override;
	static RHIVulkan* InstanceVulkan();
	const VulkanContext& GetContext();
	static VkDevice GetDevice();
	ERHIFormat GetDepthFormat() override;
	USize2D GetRenderArea() override;

	RHIBuffer* CreateBuffer(const RHIBufferDesc& desc, void* defaultData) override;
	RHITexture* CreateTexture(const RHITextureDesc& desc, void* defaultData) override;
	RHISampler* CreateSampler(const RHISamplerDesc& desc) override;
	RHIFence* CreateFence(bool sig) override;
	RHIShader* CreateShader(EShaderStageFlagBit type, const char* codeData, size_t codeSize, const char* entryFunc) override;
	RHIGraphicsPipelineState* CreateGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc) override;
	RHIComputePipelineState* CreateComputePipelineState(const RHIComputePipelineStateDesc& desc) override;
	RHIRenderPass* CreateRenderPass(const RHIRenderPassDesc& desc) override;
	RHIShaderParameterSet* CreateShaderParameterSet(const RHIShaderParemeterLayout& layout) override;
	RHICommandBuffer* CreateCommandBuffer()override;
	void SubmitCommandBuffer(RHICommandBuffer* cmd, RHIFence* fence) override;
	void SubmitCommandBuffer(TArrayView<RHICommandBuffer*> cmds, RHIFence* fence) override;
	void Present() override;
private:
	VkPipelineLayout CreatePipelineLayout(const RHIPipelineLayout& rhiLayout);
	VulkanContext m_Context;
};
