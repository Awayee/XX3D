#pragma once
#include "RHI/Public/RHIResources.h"
#include "VulkanMemory.h"
#include "Core/Public/String.h"

class RHIVulkan;

class RHIVulkanSwapChain : public RHISwapChain {
public:
	explicit RHIVulkanSwapChain(const VulkanContext* context);
	~RHIVulkanSwapChain() override;
	bool Present() override;
	void Resize(USize2D size) override;
	USize2D GetExtent() override;
private:
	enum : uint32 {
		WAIT_MAX = 0xff,
		MAX_FRAME_COUNT = 3,
	};
	const VulkanContext* m_ContextPtr;
	uint32 m_Width;
	uint32 m_Height;
	VkSwapchainKHR m_Handle;
	TVector<VkImage> m_Images;
	TVector<VkImageView> m_Views;
	uint8 m_CurFrame{ 0 };
	bool m_Prepared = false;
	struct FrameResource {
		uint32 ImageIdx;
		VkSemaphore ImageAvailableSmp{ VK_NULL_HANDLE };
		VkSemaphore PreparePresentSmp{ VK_NULL_HANDLE };
	};
	TVector<FrameResource> m_FrameRes;
	void CreateSwapChain();
	void ClearSwapChain();
};

class RHIVkBuffer: public RHIBuffer {
private:
	friend RHIVulkan;
	VkBuffer m_Buffer;
	VulkanMem m_Mem;
public:
	using RHIBuffer::RHIBuffer;
	~RHIVkBuffer()override;
	void SetName(const char* name) override;
	void UpdateData(const void* data, size_t byteSize) override;
	VkBuffer GetBuffer() { return m_Buffer; }
};

class RHIVkTexture: public RHITexture {
protected:
	friend RHIVulkan;
	VkImage m_Image;
	VkImageView m_View;
public:
	using RHITexture::RHITexture;
	virtual ~RHIVkTexture() override;
	VkImage GetImage() { return m_Image; }
	VkImageView GetView() { return m_View; }
	void SetName(const char* name) override;
};

class RHIVkTextureWithMem: public RHIVkTexture {
private:
	friend RHIVulkan;
	VulkanMem m_Mem;
public:
	using RHIVkTexture::RHIVkTexture;
	~RHIVkTextureWithMem() override;
};

class RHIVkSampler: public RHISampler{
private:
	friend RHIVulkan;
	VkSampler m_Sampler;
public:
	using RHISampler::RHISampler;
	~RHIVkSampler()override;
	VkSampler GetSampler() { return m_Sampler; }
	void SetName(const char* name) override;
};

class RHIVkFence : public RHIFence {
public:
	explicit RHIVkFence(VkFence fence);
	~RHIVkFence() override;
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
	explicit RHIVulkanGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc);
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
	explicit RHIVulkanComputePipelineState(const RHIComputePipelineStateDesc& desc);
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
