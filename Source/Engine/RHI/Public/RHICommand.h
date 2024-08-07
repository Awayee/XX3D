#pragma once
#include "RHIResources.h"
#include "Core/Public/Func.h"

// command buffer
class RHICommandBuffer {
public:
	virtual ~RHICommandBuffer() {}
	virtual void Reset() = 0;
	virtual void BeginRendering(const RHIRenderPassInfo& desc) = 0;
	virtual void EndRendering() = 0;
	virtual void BindGraphicsPipeline(RHIGraphicsPipelineState* pipeline) = 0;
	virtual void BindComputePipeline(RHIComputePipelineState* pipeline) = 0;
	virtual void BindShaderParameterSet(uint32 index, RHIShaderParameterSet* set) = 0;
	virtual void BindVertexBuffer(RHIBuffer* buffer, uint32 first, uint64 offset) = 0;
	virtual void BindIndexBuffer(RHIBuffer* buffer, uint64 offset) = 0;
	virtual void Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstIndex, uint32 firstInstance) = 0;
	virtual void DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance) = 0;
	virtual void Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ) = 0;
	virtual void ClearColorAttachment(const float* color, const IRect& rect) = 0;
	virtual void CopyBufferToTexture(RHIBuffer* buffer, RHITexture* texture, uint32 mipLevel, uint32 baseLayer, uint32 layerCount) = 0;
	virtual void CopyTextureToTexture(RHITexture* srcTex, RHITexture* dstTex, const RHITextureCopyRegion& region) = 0;
	virtual void CopyBufferToBuffer(RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, uint64 srcOffset, uint64 dstOffset, uint64 size) = 0;
	virtual void TransitionTextureState(RHITexture* texture, EResourceState stateBefore, EResourceState stateAfter, RHITextureSubDesc subDesc) = 0;
	virtual void GenerateMipmap(RHITexture* texture, uint32 levelCount, uint32 baseLayer, uint32 layerCount) = 0;

	virtual void BeginDebugLabel(const char* msg, const float* color) = 0;
	virtual void EndDebugLabel() = 0;
};
//typedef void(*CommandBufferFunc)(RCommandBuffer*);
typedef  std::function<void(RHICommandBuffer*)> CommandBufferFunc;
