#pragma once
#include "RHI/Public/RHI.h"
#include "VulkanCommon.h"
#include "Core/Public/TypeDefine.h"
#include "Core/Public/TVector.h"

class VulkanCommandMgr {
public:
	explicit VulkanCommandMgr(const VulkanContext* context);
	~VulkanCommandMgr();
	VkCommandBuffer NewCmd();
	// Add a command buffer to a list, will be submitted util call SubmitInternal.
	void Submit(VkCommandBuffer handle);
	// Add a command buffer to a list, will be freed after call SubmitInternal.
	void FreeCmd(VkCommandBuffer handle);
private:
	friend class RHIVulkan;
	const VulkanContext* m_ContextPtr;
	VkCommandPool m_Pool;
	TVector<VkCommandBuffer> m_ToSubmit;
	TVector<VkCommandBuffer> m_ToFree;
	TVector<VkSemaphore> m_Semaphores;
	void SubmitInternal();
};

class RHIVulkanCommandBuffer : public RHICommandBuffer {
public:
	RHIVulkanCommandBuffer(VkCommandBuffer handle, VulkanCommandMgr* mgr);
	~RHIVulkanCommandBuffer() override;
	void BeginRenderPass(RHIRenderPass* pass) override;
	void EndRenderPass() override;
	void CopyBufferToBuffer(RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, uint64 srcOffset, uint64 dstOffset, uint64 size) override;
	void CopyBufferToTexture(RHIBuffer* buffer, RHITexture* texture, uint32 mipLevel, uint32 baseLayer, uint32 layerCount) override;
	void CopyTextureToTexture(RHITexture* srcTex, RHITexture* dstTex, const RHITextureCopyRegion& region) override;
	void GenerateMipmap(RHITexture* texture, uint32 levelCount, uint32 baseLayer, uint32 layerCount) override;
	void BindGraphicsPipeline(RHIGraphicsPipelineState* pipeline) override;
	void BindComputePipeline(RHIComputePipelineState* pipeline) override;
	void BindVertexBuffer(RHIBuffer* buffer, uint32 first, uint64 offset) override;
	void BindIndexBuffer(RHIBuffer* buffer, uint64 offset) override;
	void Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstIndex, uint32 firstInstance) override;
	void DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance) override;
	void Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ) override;
	void ClearColorAttachment(const float* color, const IRect& rect) override;

	void BeginDebugLabel(const char* msg, const float* color) override;
	void EndDebugLabel() override;
private:
	void CmdBegin();
	VulkanCommandMgr* m_Mgr;
	VkCommandBuffer m_Handle{ VK_NULL_HANDLE };
	VkCommandPool m_Pool{ VK_NULL_HANDLE };
	VkRenderPass m_CurrentPass{ VK_NULL_HANDLE };
	uint32 m_SubPass{ 0 };
	bool m_IsBegin{ false };
};
