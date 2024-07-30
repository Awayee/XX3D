#pragma once
#include "RHI/Public/RHI.h"
#include "Core/Public/TUniquePtr.h"
#include "VulkanCommon.h"
#include "VulkanResources.h"

class VulkanContext;
class VulkanDevice;
class VulkanSwapchain;

class VulkanRHI final: public RHI {
public:
	explicit VulkanRHI(const RHIInitDesc& desc);
	~VulkanRHI()override;
	void Update() override;
	const VulkanContext* GetContext();
	VulkanDevice* GetDevice();
	VulkanSwapchain* GetSwapchain();

	ERHIFormat GetDepthFormat() override;
	USize2D GetRenderArea() override;

	RHIBufferPtr CreateBuffer(const RHIBufferDesc& desc, void* defaultData) override;
	RHITexturePtr CreateTexture(const RHITextureDesc& desc, void* defaultData) override;
	RHISamplerPtr CreateSampler(const RHISamplerDesc& desc) override;
	RHIFencePtr CreateFence(bool sig) override;
	RHIShaderPtr CreateShader(EShaderStageFlagBit type, const char* codeData, size_t codeSize, const char* entryFunc) override;
	RHIGraphicsPipelineStatePtr CreateGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc) override;
	RHIComputePipelineStatePtr CreateComputePipelineState(const RHIComputePipelineStateDesc& desc) override;
	RHIShaderParameterSetPtr CreateShaderParameterSet(const RHIShaderParemeterLayout& layout) override;
	RHICommandBufferPtr CreateCommandBuffer() override;
	void SubmitCommandBuffer(RHICommandBuffer* cmd, RHIFence* fence) override;
	void SubmitCommandBuffer(TArrayView<RHICommandBuffer*> cmds, RHIFence* fence) override;
private:
	TUniquePtr<VulkanContext> m_Context;
	TUniquePtr<VulkanDevice> m_Device;
	TUniquePtr<VulkanSwapchain> m_Swapchain;
	VkPipelineLayout CreatePipelineLayout(const RHIPipelineLayout& rhiLayout);
};
