#pragma once
#include "RHI/Public/RHIResources.h"
#include "VulkanMemory.h"
#include "Core/Public/String.h"
#include "Core/Public/TArrayView.h"

class RHIVulkan;

class RHIVulkanResource: public RHIResource {
protected:
	VkDevice m_DeviceHandle{VK_NULL_HANDLE};
};

class RHIVulkanBuffer: public RHIBuffer, public RHIVulkanResource {
private:
	friend RHIVulkan;
	VkBuffer m_Buffer;
	VulkanMem m_Mem;
public:
	RHIVulkanBuffer(const RHIBufferDesc& desc, VkBuffer buffer, VulkanMem&& mem);
	~RHIVulkanBuffer()override;
	void SetName(const char* name) override;
	void UpdateData(const void* data, size_t byteSize) override;
	VkBuffer GetBuffer() const { return m_Buffer; }
};

class RHIVulkanTexture: public RHITexture {
protected:
	VkImage m_Image;
	VkImageView m_View;
public:
	RHIVulkanTexture(const RHITextureDesc& desc, VkImage image, VkImageView view);
	RHIVulkanTexture(const RHIVulkanTexture&) = delete;
	virtual ~RHIVulkanTexture() override;
	VkImage GetImage() const { return m_Image; }
	VkImageView GetView() const { return m_View; }
	void SetName(const char* name) override;
};

class RHIVulkanTextureWithMem: public RHIVulkanTexture {
private:
	VulkanMem m_Mem;
public:
	RHIVulkanTextureWithMem(const RHITextureDesc& desc, VkImage image, VkImageView view, VulkanMem&& memory);
	~RHIVulkanTextureWithMem() override;
};

class RHIVulkanSampler: public RHISampler{
private:
	friend RHIVulkan;
	VkSampler m_Sampler;
public:
	RHIVulkanSampler(const RHISamplerDesc& desc, VkSampler sampler);
	~RHIVulkanSampler()override;
	VkSampler GetSampler() { return m_Sampler; }
	void SetName(const char* name) override;
};

class RHIVulkanFence : public RHIFence {
public:
	explicit RHIVulkanFence(VkFence fence);
	~RHIVulkanFence() override;
	void Wait() override;
	void Reset() override;
	void SetName(const char* name) override;
	VkFence GetFence() const { return m_Handle; }
private:
	static constexpr uint32 WAIT_MAX = 0xffff;
	VkFence m_Handle{VK_NULL_HANDLE};
};

class RHIVulkanShader: public RHIShader {
public:
	RHIVulkanShader(EShaderStageFlagBit type, const char* code, size_t codeSize, const char* funcName);
	~RHIVulkanShader() override;
	void SetName(const char* name) override;
	XX_NODISCARD VkPipelineShaderStageCreateInfo GetShaderStageInfo() const;
private:
	XXString m_EntryName;
	VkShaderModule m_ShaderModule;
};

class RHIVulkanGraphicsPipelineState : public RHIGraphicsPipelineState {
public:
	explicit RHIVulkanGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc, VkPipelineLayout layout);
	~RHIVulkanGraphicsPipelineState() override;
	void SetName(const char* name) override;
	VkPipeline GetPipelineHandle(VkRenderPass pass, uint32 subPass);
	void CreatePipelineHandle(VkRenderPass pass, uint32 subPass);
private:
	friend RHIVulkan;
	XXString m_Name;
	VkPipelineLayout m_PipelineLayout{VK_NULL_HANDLE};
	VkPipeline m_Pipeline{VK_NULL_HANDLE};
};

class RHIVulkanComputePipelineState: public RHIComputePipelineState {
public:
	explicit RHIVulkanComputePipelineState(const RHIComputePipelineStateDesc& desc, VkPipelineLayout layout);
	~RHIVulkanComputePipelineState() override;
	void SetName(const char* name) override;
	VkPipeline GetPipelineHandle() const { return m_Pipeline; }
private:
	friend RHIVulkan;
	VkPipelineLayout m_PipelineLayout{ VK_NULL_HANDLE };
	VkPipeline m_Pipeline{ VK_NULL_HANDLE };
};

// layout manager in one pass
class VulkanLayoutMgr {
public:
	VkImageLayout GetCurrentLayout(VkImage handle) const;
	VkImageLayout GetCurrentLayout(const RHITexture* texture) const;
	VkImageLayout GetFinalLayout(const RHITexture* texture) const;
};

class RHIVulkanRenderPass: public RHIRenderPass {
public:
	explicit RHIVulkanRenderPass(const RHIRenderPassDesc& desc);
	~RHIVulkanRenderPass() override;
	void SetName(const char* name) override;
	VkRenderPass GetRenderPass() { return m_RenderPass; }
	VkFramebuffer GetFramebuffer() { return m_Framebuffer; }
private:
	VkRenderPass m_RenderPass;
	VkFramebuffer m_Framebuffer;
	void DestroyHandle();
	void CreateHandle(const VulkanLayoutMgr& layoutMgr);
};
