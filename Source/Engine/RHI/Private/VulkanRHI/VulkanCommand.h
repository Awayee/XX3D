#pragma once
#include "RHI/Public/RHI.h"
#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanResources.h"
#include "Core/Public/TArray.h"
#include "Core/Public/TUniquePtr.h"

class VulkanCommandContext;
class VulkanPipelineDescriptorSetCache;

class VulkanStagingBuffer : public VulkanBuffer {
public:
	VulkanStagingBuffer(uint32 bufferSize, VulkanDevice* device);
	uint32 GetCreateFrame() const { return m_CreateFrame; }

private:
	uint32 m_CreateFrame;
};

class VulkanUploader {
public:
	explicit VulkanUploader(VulkanDevice* device);
	~VulkanUploader();
	VulkanStagingBuffer* AcquireBuffer(uint32 bufferSize);// return a staging buffer, which will be released after cmd executing.
	void BeginFrame();
private:
	VulkanDevice* m_Device;
	TArray<TUniquePtr<VulkanStagingBuffer>> m_StagingBuffers;// Buffers to upload in current frame.
};

class VulkanCommandBuffer : public RHICommandBuffer {
public:
	VulkanCommandBuffer(VulkanCommandContext* context, VkCommandBuffer handle, EQueueType queue);
	~VulkanCommandBuffer() override;
	VkCommandBuffer GetHandle() const { return m_Handle; }
	EQueueType GetQueueType()const { return m_QueueType; }
	void Reset() override;
	void Close() override;
	void BeginRendering(const RHIRenderPassInfo& info) override;
	void EndRendering() override;
	void BindGraphicsPipeline(RHIGraphicsPipelineState* pipeline) override;
	void BindComputePipeline(RHIComputePipelineState* pipeline) override;
	void SetShaderParam(uint32 setIndex, uint32 bindIndex, const RHIShaderParam& parameter) override;
	void BindVertexBuffer(RHIBuffer* buffer, uint32 slot, uint64 offset) override;
	void BindVertexBuffer(const RHIDynamicBuffer& buffer, uint32 slot, uint32 offset) override;
	void BindIndexBuffer(RHIBuffer* buffer, uint64 offset) override;
	void SetViewport(FRect rect, float minDepth, float maxDepth) override;
	void SetScissor(Rect rect) override;
	void Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstIndex, uint32 firstInstance) override;
	void DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance) override;
	void Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ) override;
	void DrawIndirect(const RHIDynamicBuffer& buffer, uint32 drawCount) override;
	void DrawIndexedIndirect(const RHIDynamicBuffer& buffer, uint32 drawCount) override;
	void ClearColorTarget(uint32 targetIndex, const float* color, const IRect& rect) override;
	void CopyBufferToBuffer(RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, uint32 srcOffset, uint32 dstOffset, uint32 byteSize) override;
	void CopyBufferToTexture(RHIBuffer* buffer, RHITexture* texture, RHITextureSubRes dstSubRes, IOffset3D dstOffset) override;
	void CopyTextureToTexture(RHITexture* srcTex, RHITexture* dstTex, const RHITextureCopyRegion& region) override;
	void TransitionTextureState(RHITexture* texture, EResourceState stateBefore, EResourceState stateAfter, RHITextureSubRes subRes) override;
	void GenerateMipmap(RHITexture* texture, uint8 mipSize, uint16 arrayIndex, uint16 arraySize, ETextureViewFlags viewFlags) override;
	void BeginDebugLabel(const char* msg, const float* color) override;
	void EndDebugLabel() override;
private:
	friend VulkanCommandContext;
	VulkanCommandContext* m_Owner;
	VkCommandBuffer m_Handle{ VK_NULL_HANDLE };
	EQueueType m_QueueType;
	TUniquePtr<VulkanPipelineDescriptorSetCache> m_PipelineDescriptorSetCache;
	VkViewport m_Viewport {};
	VkRect2D m_Scissor {};
	bool m_HasBegun{ false };
	void CheckBegin();
	void CheckEnd();
	// call before draw/dispatch
	void PrepareDraw();
};

typedef TArray<VulkanCommandBuffer*> VulkanCommandSubmission;

class SemaphoreCache {
public:
	SemaphoreCache(VkDevice device): m_Device(device), m_CurrentIdx(0){}
	~SemaphoreCache();
	VkSemaphore Get();
	void Reset();
	void GC();
private:
	TArray<VkSemaphore> m_Semaphores;
	VkDevice m_Device;
	uint32 m_CurrentIdx;
};

class VulkanCommandContext {
public:
	NON_COPYABLE(VulkanCommandContext);
	NON_MOVEABLE(VulkanCommandContext);
	explicit VulkanCommandContext(VulkanDevice* device);
	~VulkanCommandContext();
	RHICommandBufferPtr AllocateCommandBuffer(EQueueType queue);
	void FreeCommandBuffer(VulkanCommandBuffer* cmd);
	// Input command buffers for parallel execution.
	void SubmitCommandBuffers(TArrayView<VulkanCommandBuffer*> cmds, EQueueType queue, VkSemaphore waitSemaphore, VkFence completeFence);
	void BeginFrame();
	// Get the semaphores of commands last submitted.
	VkSemaphore GetLastSubmissionSemaphore();
	// Get the command buffer for upload, always exists.
	VulkanCommandBuffer* GetUploadCmd(EQueueType queue);

	VulkanDevice* GetDevice();
private:
	VulkanDevice* m_Device;
	TStaticArray<VkCommandPool, EnumCast(EQueueType::Count)> m_CommandPools;
	TStaticArray<TUniquePtr<VulkanCommandBuffer>, EnumCast(EQueueType::Count)> m_UploadCmds; // TODO double buffers
	SemaphoreCache m_SemaphoreCache;
	VkSemaphore m_LastSemaphore;
	VkCommandPool GetCommandPool(EQueueType queue);
};
