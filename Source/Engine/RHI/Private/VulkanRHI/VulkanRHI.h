#pragma once
#include "RHI/Public/RHI.h"
#include "Core/Public/TUniquePtr.h"
#include "VulkanCommon.h"
#include "VulkanResources.h"

class VulkanContext;
class VulkanDevice;
class VulkanViewport;

class VulkanRHI final: public RHI {
public:
	explicit VulkanRHI(const RHIInitDesc& desc);
	~VulkanRHI()override;
	void BeginFrame() override;
	const VulkanContext* GetContext();
	VulkanDevice* GetDevice();
	ERHIFormat GetDepthFormat() override;
	RHIViewport* GetViewport() override;
	RHIBufferPtr CreateBuffer(const RHIBufferDesc& desc) override;
	RHITexturePtr CreateTexture(const RHITextureDesc& desc) override;
	RHISamplerPtr CreateSampler(const RHISamplerDesc& desc) override;
	RHIFencePtr CreateFence(bool sig) override;
	RHIShaderPtr CreateShader(EShaderStageFlagBit type, const char* codeData, size_t codeSize, const XString& entryFunc) override;
	RHIGraphicsPipelineStatePtr CreateGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc) override;
	RHIComputePipelineStatePtr CreateComputePipelineState(const RHIComputePipelineStateDesc& desc) override;
	RHICommandBufferPtr CreateCommandBuffer() override;
	void SubmitCommandBuffer(RHICommandBuffer* cmd, RHIFence* fence, bool bPresent) override;
	void SubmitCommandBuffers(TArrayView<RHICommandBuffer*> cmds, RHIFence* fence, bool bPresent) override;
private:
	TUniquePtr<VulkanContext> m_Context;
	TUniquePtr<VulkanDevice> m_Device;
	TUniquePtr<VulkanViewport> m_Viewport;
};
