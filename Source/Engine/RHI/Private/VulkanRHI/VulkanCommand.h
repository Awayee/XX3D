#pragma once
#include "RHI/Public/RHI.h"
#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "Core/Public/TArray.h"
#include "Core/Public/TUniquePtr.h"

class VulkanCommandContext;
class VulkanPipelineStateContainer;

class VulkanCommandBuffer : public RHICommandBuffer {
public:
	VulkanCommandBuffer(VulkanCommandContext* context, VkCommandBuffer handle, VkSemaphore smp);
	~VulkanCommandBuffer() override;
	VkCommandBuffer GetHandle() const { return m_Handle; }
	VkSemaphore GetSemaphore() const { return m_Semaphore; }
	void Reset() override;
	void BeginRendering(const RHIRenderPassInfo& info) override;
	void EndRendering() override;
	void BindGraphicsPipeline(RHIGraphicsPipelineState* pipeline) override;
	void BindComputePipeline(RHIComputePipelineState* pipeline) override;
	void SetShaderParameter(uint32 setIndex, uint32 bindIndex, const RHIShaderParam& parameter) override;
	void BindVertexBuffer(RHIBuffer* buffer, uint32 first, uint64 offset) override;
	void BindIndexBuffer(RHIBuffer* buffer, uint64 offset) override;
	void SetViewport(FRect rect, float minDepth, float maxDepth) override;
	void SetScissor(IURect rect) override;
	void Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstIndex, uint32 firstInstance) override;
	void DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance) override;
	void Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ) override;
	void ClearColorAttachment(const float* color, const IRect& rect) override;
	void CopyBufferToBuffer(RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, uint64 srcOffset, uint64 dstOffset, uint64 size) override;
	void CopyBufferToTexture(RHIBuffer* buffer, RHITexture* texture, uint32 mipLevel, uint32 baseLayer, uint32 layerCount) override;
	void CopyTextureToTexture(RHITexture* srcTex, RHITexture* dstTex, const RHITextureCopyRegion& region) override;
	void TransitionTextureState(RHITexture* texture, EResourceState stateBefore, EResourceState stateAfter, RHITextureSubDesc subDesc) override;
	void GenerateMipmap(RHITexture* texture, uint32 levelCount, uint32 baseLayer, uint32 layerCount) override;
	void BeginDebugLabel(const char* msg, const float* color) override;
	void EndDebugLabel() override;
private:
	friend VulkanCommandContext;
	VulkanCommandContext* m_Owner;
	VkCommandBuffer m_Handle{ VK_NULL_HANDLE };
	TUniquePtr<VulkanPipelineStateContainer> m_PipelineStateContainer;
	VkViewport m_Viewport {};
	VkRect2D m_Scissor {};
	// signal semaphore
	VkSemaphore m_Semaphore;
	VkPipelineStageFlags m_StageMask;
	bool m_HasBegun{ false };
	void CheckBegin();
	void CheckEnd();
	// call before draw/dispatch
	void PrepareDrawOrDispatch();
};

typedef TArray<VulkanCommandBuffer*> VulkanCommandSubmission;

class VulkanCommandContext {
public:
	NON_COPYABLE(VulkanCommandContext);
	NON_MOVEABLE(VulkanCommandContext);
	explicit VulkanCommandContext(VulkanDevice* device);
	~VulkanCommandContext();
	RHICommandBufferPtr AllocateCommandBuffer();
	void FreeCommandBuffer(VulkanCommandBuffer* cmd);
	// Input command buffer(s) for parallel execution.
	void SubmitCommandBuffer(VulkanCommandBuffer* cmd, VkSemaphore waitSemaphore, VkFence completeFence);
	void SubmitCommandBuffers(TArrayView<VulkanCommandBuffer*> cmds, VkSemaphore waitSemaphore, VkFence completeFence);
	void BeginFrame();
	// Get the last submitted command buffer(s) for next submission.
	const VulkanCommandSubmission& GetLastSubmission();
	// Get the semaphores of commands last submitted.
	const void GetLastSubmissionSemaphores(TArray<VkSemaphore>& outSmps);
	// Get the command buffer for upload, always exists.
	VulkanCommandBuffer* GetUploadCmd();

	VulkanDevice* GetDevice();
private:
	VulkanDevice* m_Device;
	VkCommandPool m_CommandPool;
	TArray<VulkanCommandSubmission> m_Submissions;
	TUniquePtr<VulkanCommandBuffer> m_UploadCmd;
};
