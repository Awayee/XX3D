#pragma once
#include "RHIResources.h"
#include "Core/Public/Func.h"

// command buffer
class RHICommandBuffer {
public:
	virtual ~RHICommandBuffer() {}
	virtual void Reset() = 0;
	virtual void BeginRendering(const RHIRenderPassInfo& info) = 0;
	virtual void EndRendering() = 0;
	virtual void BindGraphicsPipeline(RHIGraphicsPipelineState* pipeline) = 0;
	virtual void BindComputePipeline(RHIComputePipelineState* pipeline) = 0;
	virtual void SetShaderParam(uint32 setIndex, uint32 bindIndex, const RHIShaderParam& parameter) = 0;
	virtual void BindVertexBuffer(RHIBuffer* buffer, uint32 first, uint64 offset) = 0;
	virtual void BindIndexBuffer(RHIBuffer* buffer, uint64 offset) = 0;
	virtual void SetViewport(FRect rect, float minDepth, float maxDepth) = 0;
	virtual void SetScissor(Rect rect) = 0;
	virtual void Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstIndex, uint32 firstInstance) = 0;
	virtual void DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance) = 0;
	virtual void Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ) = 0;
	virtual void ClearColorTarget(uint32 targetIndex, const float* color, const IRect& rect) = 0;
	virtual void CopyBufferToTexture(RHIBuffer* buffer, RHITexture* texture, RHITextureSubRes dstSubRes, IOffset3D dstOffset) = 0;
	virtual void CopyTextureToTexture(RHITexture* srcTex, RHITexture* dstTex, const RHITextureCopyRegion& region) = 0;
	virtual void CopyBufferToBuffer(RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, uint32 srcOffset, uint32 dstOffset, uint32 byteSize) = 0;
	virtual void TransitionTextureState(RHITexture* texture, EResourceState stateBefore, EResourceState stateAfter, RHITextureSubRes subDesc={}) = 0;
	virtual void GenerateMipmap(RHITexture* texture, uint8 mipSize, uint16 arrayIndex, uint16 arraySize, ETextureViewFlags viewFlags) = 0;

	virtual void BeginDebugLabel(const char* msg, const float* color) = 0;
	virtual void EndDebugLabel() = 0;
};
