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
	~RHIVkFence() override;
	void Wait() override;
	void Reset() override;
	void SetName(const char* name) override;
private:
	friend RHIVulkan;
	static constexpr uint32 WAIT_MAX = 0xffff;
	VkFence m_Handle;
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
private:
	VkRenderPass m_RenderPass;
	VkFramebuffer m_Framebuffer;
	void DestroyHandle();
	void CreateHandle(const VulkanLayoutMgr& layoutMgr);
};



class RHIVulkanCommandBuffer : public RHICommandBuffer {
public:
	RHIVulkanCommandBuffer(VkCommandBuffer cmd, VkCommandPool pool);
	~RHIVulkanCommandBuffer() override;
	void BeginRenderPass(const RHIRenderPassDesc& passInfo) override;
	void EndRenderPass() override;
	void CopyBufferToBuffer(RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, uint64 srcOffset, uint64 dstOffset, uint64 size) override;
	void CopyBufferToTexture(RHIBuffer* buffer, RHITexture* texture, uint32 mipLevel, uint32 baseLayer, uint32 layerCount) override;
	void CopyTextureToTexture(RHITexture* srcTex, RHITexture* dstTex, const RHITextureCopyRegion& region) override;
	void TransitionTextureLayout(RHITexture* texture, RImageLayout oldLayout, RImageLayout newLayout, uint32 baseMipLevel, uint32 levelCount, uint32 baseLayer, uint32 layerCount, EImageAspectFlags aspect) override;
	void GenerateMipmap(RHITexture* texture, uint32 levelCount, EImageAspectFlags aspect, uint32 baseLayer, uint32 layerCount) override;
	void BindGraphicsPipeline(RHIGraphicsPipelineState* pipeline) override;
	void BindComputePipeline(RHIComputePipelineState* pipeline) override;
	void BindVertexBuffer(RHIBuffer* buffer, uint32 first, uint64 offset) override;
	void BindIndexBuffer(RHIBuffer* buffer, uint64 offset) override;
	void Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstIndex, uint32 firstInstance) override;
	void DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance) override;
	void Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ) override;
	void ClearAttachment(EImageAspectFlags aspect, const float* color, const IRect& rect) override;
	void BeginDebugLabel(const char* msg, const float* color) override;
	void EndDebugLabel() override;
private:
	void CmdBegin();
	VkCommandBuffer m_VkCmd{ VK_NULL_HANDLE };
	VkCommandPool m_Pool{ VK_NULL_HANDLE };
	VkRenderPass m_CurrentPass{ VK_NULL_HANDLE };
	uint32 m_SubPass{ 0 };
	bool m_IsBegin{ false };
};

class RHIVulkanShaderParameterSet: public RHIShaderParameterSet {
public:
	explicit RHIVulkanShaderParameterSet(const RHIShaderBindingLayout& layout);
	~RHIVulkanShaderParameterSet() override;
	void SetUniformBuffer(uint32 binding, RHIBuffer* buffer) override;
	void SetSampler(uint32 binding, RHISampler* sampler) override;
	void SetTexture(uint32 binding, RHITexture* image) override;
};
