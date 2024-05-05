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

class RHIVulkanBuffer: public RHIBuffer {
private:
	friend RHIVulkan;
};

class RHIVulkanBufferWithMem: public RHIBuffer{
public:
	RHIVulkanBufferWithMem(const RHIBufferDesc& desc, VkBuffer buffer, VulkanMem&& mem);
	~RHIVulkanBufferWithMem()override;
	void SetName(const char* name) override;
	void UpdateData(const void* data, size_t byteSize) override;
	VkBuffer GetBuffer() const { return m_Buffer; }
private:
	friend RHIVulkan;
	VkBuffer m_Buffer;
	VulkanMem m_Mem;
};

class RHIVulkanTexture: public RHITexture {
public:
	RHIVulkanTexture(const RHITextureDesc& desc, VkImage image, VkImageView view);
	virtual ~RHIVulkanTexture() override;
	VkImage GetImage() const { return m_Image; }
	VkImageView GetView() const { return m_View; }
	void SetName(const char* name) override;
	void UpdateData(RHITextureOffset offset, USize3D size, const void* data) override;
protected:
	VkImage m_Image;
	VkImageView m_View;
};

class RHIVulkanTextureWithMem: public RHIVulkanTexture {
public:
	RHIVulkanTextureWithMem(const RHITextureDesc& desc, VkImage image, VkImageView view, VulkanMem&& memory);
	~RHIVulkanTextureWithMem() override;
private:
	VulkanMem m_Mem;
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
	VkPipelineLayout GetLayoutHandle() const { return m_PipelineLayout; }
private:
	void CreatePipelineHandle(VkRenderPass pass, uint32 subPass);
	void Release();
	friend RHIVulkan;
	XXString m_Name;
	VkRenderPass m_TargetRenderPass{VK_NULL_HANDLE};
	uint32 m_TargetSubPass{0};
	VkPipelineLayout m_PipelineLayout{VK_NULL_HANDLE};
	VkPipeline m_Pipeline{VK_NULL_HANDLE};
};

class RHIVulkanComputePipelineState: public RHIComputePipelineState {
public:
	explicit RHIVulkanComputePipelineState(const RHIComputePipelineStateDesc& desc, VkPipelineLayout layout);
	~RHIVulkanComputePipelineState() override;
	void SetName(const char* name) override;
	VkPipeline GetPipelineHandle() const { return m_Pipeline; }
	VkPipelineLayout GetLayoutHandle() const { return m_PipelineLayout; }
private:
	friend RHIVulkan;
	VkPipelineLayout m_PipelineLayout{ VK_NULL_HANDLE };
	VkPipeline m_Pipeline{ VK_NULL_HANDLE };
};

// layout manager in one pass
struct VulkanImageLayoutWrap {
	VkImageLayout InitialLayout{ VK_IMAGE_LAYOUT_UNDEFINED };
	VkImageLayout CurrentLayout{ VK_IMAGE_LAYOUT_UNDEFINED };
	VkImageLayout FinalLayout{ VK_IMAGE_LAYOUT_UNDEFINED };
};
class VulkanImageLayoutMgr {
public:
	VulkanImageLayoutMgr() = default;
	~VulkanImageLayoutMgr() = default;
	VulkanImageLayoutWrap* GetLayout(RHITexture* tex);
	const VulkanImageLayoutWrap* GetLayout(RHITexture* tex) const;
private:
	std::unordered_map<VkImage, VulkanImageLayoutWrap> m_ImageLayoutMap;
};

class RHIVulkanRenderPass: public RHIRenderPass {
public:
	explicit RHIVulkanRenderPass(const RHIRenderPassDesc& desc);
	~RHIVulkanRenderPass() override;
	void SetName(const char* name) override;
	void ResolveImageLayout(const VulkanImageLayoutMgr* layoutMgr);
	VkRenderPass GetRenderPass() const { return m_RenderPass; }
	uint32 GetSubPass() const { return m_SubPass; }
	VkFramebuffer GetFramebuffer() const { return m_Framebuffer; }
private:
	VkRenderPass m_RenderPass;
	uint32 m_SubPass;
	VkFramebuffer m_Framebuffer;
	const VulkanImageLayoutMgr* m_ImageLayoutMgr;
	void DestroyHandle();
	void CreateHandle();
};
