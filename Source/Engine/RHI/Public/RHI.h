#pragma once
#include "RHIResources.h"
#include "Core/Public/TArrayView.h"
#include "Core/Public/Func.h"
#include "Core/Public/String.h"

// command buffer
class RHICommandBuffer {
public:
	virtual ~RHICommandBuffer() {}
	virtual void BeginRenderPass(RHIRenderPass* pass) = 0;
	virtual void EndRenderPass() = 0;
	virtual void BindGraphicsPipeline(RHIGraphicsPipelineState* pipeline) = 0;
	virtual void BindComputePipeline(RHIComputePipelineState* pipeline) = 0;
	virtual void BindVertexBuffer(RHIBuffer* buffer, uint32 first, uint64 offset) = 0;
	virtual void BindIndexBuffer(RHIBuffer* buffer, uint64 offset) = 0;
	virtual void Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstIndex, uint32 firstInstance) = 0;
	virtual void DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance) = 0;
	virtual void Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ) = 0;
	virtual void ClearColorAttachment(const float* color, const IRect& rect) = 0;
	virtual void CopyBufferToTexture(RHIBuffer* buffer, RHITexture* texture, uint32 mipLevel, uint32 baseLayer, uint32 layerCount) = 0;
	virtual void CopyTextureToTexture(RHITexture* srcTex, RHITexture* dstTex, const RHITextureCopyRegion& region) = 0;
	virtual void CopyBufferToBuffer(RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, uint64 srcOffset, uint64 dstOffset, uint64 size) = 0;
	virtual void TextureBarrier(RHITexture* texture, RHITextureSubDesc subDesc, EResourceState stateBefore, EResourceState stateAfter) = 0;
	virtual void GenerateMipmap(RHITexture* texture, uint32 levelCount, uint32 baseLayer, uint32 layerCount) = 0;

	virtual void BeginDebugLabel(const char* msg, const float* color) = 0;
	virtual void EndDebugLabel() = 0;
};

typedef void(*DebugFunc)(const char*);
//typedef void(*CommandBufferFunc)(RCommandBuffer*);
typedef  std::function<void(RHICommandBuffer*)> CommandBufferFunc;


enum class ERHIType : uint8 {
	Vulkan,
	DX12,
	DX11,
	OpenGL,
	Invalid
};

struct RHIInitDesc {
	ERHIType RHIType;
	XXString AppName;
	void* WindowHandle;
	bool EnableDebug;
	bool IntegratedGPU;
};

class RHI
{
public:
	static RHI* Instance();
	static void Initialize(const RHIInitDesc& desc);
	static void Release();
	virtual ERHIFormat GetDepthFormat() = 0;
	virtual RHISwapChain* GetSwapChain() = 0;

	// cmd
	virtual void ImmediateSubmit(const CommandBufferFunc& func) = 0;
	virtual int QueueSubmitPresent(RHICommandBuffer* cmd, uint8 frameIndex, RHIFence* fence) = 0; // return -1 if out of date

	virtual RHIBuffer* CreateBuffer(const RHIBufferDesc& desc) = 0;
	virtual RHITexture* CreateTexture(const RHITextureDesc& desc) = 0;
	virtual RHISampler* CreateSampler(const RHISamplerDesc& desc) = 0;
	virtual RHIFence* CreateFence(bool sig = true) = 0;
	virtual RHIShader* CreateShader(EShaderStageFlagBit type, const char* codeData, size_t codeSize, const char* entryFunc) = 0;
	virtual RHIGraphicsPipelineState* CreateGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc) = 0;
	virtual RHIComputePipelineState* CreateComputePipelineState(const RHIComputePipelineStateDesc& desc) = 0;
	virtual RHIRenderPass* CreateRenderPass(const RHIRenderPassDesc& desc) = 0;

	virtual RHICommandBuffer* CreateCommandBuffer() = 0;
	virtual void SubmitCommandBuffer(TArrayView<RHICommandBuffer*> cmds, RHIFence* fence) = 0;

	RHI() = default;
	RHI(const RHI&) = delete;
	RHI(RHI&&) = delete;
	RHI* operator=(const RHI&) = delete;
protected:
	virtual ~RHI() = 0;
private:
	static RHI* s_Instance;
};

#define GET_RHI(x) Engine::RHI* x = Engine::RHI::Instance()
