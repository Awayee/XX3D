#pragma once
#include "RHIStructs.h"
#include "RHIResources.h"
#include "Core/Public/TypeDefine.h"
#include "Core/Public/BaseStructs.h"

namespace Engine{

	class RWindowHandle {
	};

	class RRenderPass {
	public:
		virtual void SetAttachment(uint32 idx, RHITexture* texture) = 0;
		virtual void SetClearValue(uint32 idx, const RSClear& clear) = 0;
	};

	class RPipeline {
	protected:
		RPipelineType m_Type;
	public:
		RPipelineType GetType() { return m_Type; }
	};

	class RFramebuffer {
	protected:
		USize2D m_Extent;
	public:
		const USize2D& Extent() const { return m_Extent; }
	};

	class RDescriptorSetLayout {
		
	};

	class RPipelineLayout {
		
	};

	class RDescriptorSet {
	public:
		virtual ~RDescriptorSet() = default;
		virtual void SetUniformBuffer(uint32 binding, RHIBuffer* buffer) = 0;
		virtual void SetImageSampler(uint32 binding, RHISampler* sampler, RHITexture* image) = 0;
		virtual void SetTexture(uint32 binding, RHITexture* image) = 0;
		virtual void SetSampler(uint32 binding, RHISampler* sampler) = 0;
		virtual void SetInputAttachment(uint32 binding, RHITexture* image) = 0;
	};
	class RSemaphore {
	public:
		virtual ~RSemaphore() = default;
	};

	// cmd
	class RHICommandBuffer {
	public:
		virtual ~RHICommandBuffer() {}
		virtual void Begin(RCommandBufferUsageFlags flags) = 0;
		virtual void End() = 0;
		virtual void BeginRenderPass(RRenderPass* pass, RFramebuffer* framebuffer, const URect& area) = 0;
		virtual void NextSubpass() = 0;
		virtual void EndRenderPass() = 0;
		virtual void CopyBufferToTexture(RHIBuffer* buffer, RHITexture* texture, uint32 mipLevel, uint32 baseLayer, uint32 layerCount) = 0;
		virtual void CopyTextureToTexture(RHITexture* srcTex, RHITexture* dstTex, const RSTextureCopyRegion& region) = 0;
		virtual void BindPipeline(RPipeline* pipeline) = 0;
		virtual void BindDescriptorSet(RPipelineLayout* layout, RDescriptorSet* descriptorSet, uint32 setIdx, RPipelineType pipelineType) = 0;
		virtual void BindVertexBuffer(RHIBuffer* buffer, uint32 first, uint64 offset) = 0;
		virtual void BindIndexBuffer(RHIBuffer* buffer, uint64 offset) = 0;
		virtual void Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstIndex, uint32 firstInstance) = 0;
		virtual void DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance) = 0;
		virtual void Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ) = 0;
		virtual void ClearAttachment(RImageAspectFlags aspect, const float* color, const URect& rect) = 0;
		virtual void CopyBufferToBuffer(RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, uint64 srcOffset, uint64 dstOffset, uint64 size) = 0;
		virtual void TransitionTextureLayout(RHITexture* texture, RImageLayout oldLayout, RImageLayout newLayout, uint32 baseMipLevel, uint32 levelCount, uint32 layer, uint32 layerCount, RImageAspectFlags aspect) = 0;
		virtual void GenerateMipmap(RHITexture* texture, uint32 levelCount, RImageAspectFlags aspect, uint32 baseLayer, uint32 layerCount) = 0;
		virtual void BeginDebugLabel(const char* msg, const float* color) = 0;
		virtual void EndDebugLabel() = 0;
	};
} // namespace RHI