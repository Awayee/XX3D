#pragma once
#include "RHI/Public/RHI.h"
#include "VulkanCommon.h"
#include "Core/Public/TypeDefine.h"
#include "Core/Public/TVector.h"
#include "Core/Public/TUniquePtr.h"
#include "Core/Public/TSingleton.h"

class VulkanImageLayoutMgr;

class SemaphoreMgr {
public:
	SemaphoreMgr() = default;
	~SemaphoreMgr();
	void Initialize(VkDevice device, uint32 maxSize);
	uint32 Allocate(); // return index of a semaphore in array
	VkSemaphore& Get(uint32 idx);
	void ResetSmps(); // reset allocates
	void Clear(); // clear all semaphores
private:
	VkDevice m_Device {VK_NULL_HANDLE};
	TVector<VkSemaphore> m_Semaphores;
	uint32 m_CurIndex {0};
};

class VulkanCommandMgr: public TSingleton<VulkanCommandMgr> {
public:
	explicit VulkanCommandMgr(const VulkanContext* context);
	~VulkanCommandMgr();
	VkCommandBuffer NewCmd();
	// Add a command buffer to a list, will be submitted util Submit is called.
	// a batch of Commands in a calling will run parallel, multi batch of commands will run in order of calling.
	void AddGraphicsSubmit(TConstArrayView<VkCommandBuffer> cmds, VkFence fence, bool toPresent=false);
	// Add a command buffer to a list, will be freed after Update.
	void FreeCmd(VkCommandBuffer handle);
	// submit cmds every frame, return a semaphore of completing all cmds
	VkSemaphore Submit(VkSemaphore presentWaitSmp);
private:
	friend TSingleton<VulkanCommandMgr>;
	const VulkanContext* m_ContextPtr;
	VkCommandPool m_Pool;
	SemaphoreMgr m_SmpMgr;
	TVector<VkCommandBuffer> m_CmdsToFree;
	TVector<VkCommandBuffer> m_CmdsToSubmit;

	// record cmd commit info
	struct SubmitInfo {
		uint32 WaitSmpIdx;
		VkPipelineStageFlags WaitStage;
		// a cmd mast have signal semaphore
		uint32 SignalSmpIdx;
		uint32 CmdStartIdx;
		uint32 CmdCount;
		bool ToPresent;
		VkFence Fence;
	};
	TVector<SubmitInfo> m_SubmitInfos;
};

class RHIVulkanCommandBuffer : public RHICommandBuffer {
public:
	RHIVulkanCommandBuffer(VkCommandBuffer handle);
	~RHIVulkanCommandBuffer() override;
	XX_NODISCARD VkCommandBuffer GetHandle() const { return m_Handle; }
	void BeginRenderPass(RHIRenderPass* pass) override;
	void EndRenderPass() override;
	void BindGraphicsPipeline(RHIGraphicsPipelineState* pipeline) override;
	void BindComputePipeline(RHIComputePipelineState* pipeline) override;
	void BindShaderParameterSet(uint32 index, RHIShaderParameterSet* set) override;
	void BindVertexBuffer(RHIBuffer* buffer, uint32 first, uint64 offset) override;
	void BindIndexBuffer(RHIBuffer* buffer, uint64 offset) override;
	void Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstIndex, uint32 firstInstance) override;
	void DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance) override;
	void Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ) override;
	void ClearColorAttachment(const float* color, const IRect& rect) override;
	void CopyBufferToBuffer(RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, uint64 srcOffset, uint64 dstOffset, uint64 size) override;
	void CopyBufferToTexture(RHIBuffer* buffer, RHITexture* texture, uint32 mipLevel, uint32 baseLayer, uint32 layerCount) override;
	void CopyTextureToTexture(RHITexture* srcTex, RHITexture* dstTex, const RHITextureCopyRegion& region) override;
	void ResourceBarrier(RHITexture* texture, RHITextureSubDesc subDesc, EResourceState stateBefore, EResourceState stateAfter) override;
	void GenerateMipmap(RHITexture* texture, uint32 levelCount, uint32 baseLayer, uint32 layerCount) override;

	void BeginDebugLabel(const char* msg, const float* color) override;
	void EndDebugLabel() override;
private:
	void CmdBegin();
	VulkanCommandMgr* m_Owner;
	VkCommandBuffer m_Handle{ VK_NULL_HANDLE };
	TUniquePtr<VulkanImageLayoutMgr> m_ImageLayoutMgr;
	// pass info
	VkRenderPass m_CurrentPass{ VK_NULL_HANDLE };
	uint32 m_SubPass{ 0 };
	// pipeline info
	VkPipeline m_CurrentPipeline{VK_NULL_HANDLE};
	VkPipelineLayout m_CurrentPipelineLayout{VK_NULL_HANDLE};
	VkPipelineBindPoint m_CurrentPipelineBindPoint;

	bool m_IsBegin{ false };
};
