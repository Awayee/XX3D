#pragma once
#include "RHI/Public/RHICommand.h"
#include "Core/Public/TArrayView.h"
#include "Core/Public/String.h"
#include "Core/Public/TUniquePtr.h"

typedef void(*DebugFunc)(const char*);

struct RHIInitDesc {
	XString AppName;
	bool EnableDebug;
	bool IntegratedGPU;
	void* WindowHandle;
	USize2D WindowSize;
};

// type defines
typedef TUniquePtr<RHICommandBuffer>             RHICommandBufferPtr;
typedef TUniquePtr<RHIBuffer>                    RHIBufferPtr;
typedef TUniquePtr<RHITexture>                   RHITexturePtr;
typedef TUniquePtr<RHISampler>                   RHISamplerPtr;
typedef TUniquePtr<RHIFence>                     RHIFencePtr;
typedef TUniquePtr<RHIShader>                    RHIShaderPtr;
typedef TUniquePtr<RHIGraphicsPipelineState>     RHIGraphicsPipelineStatePtr;
typedef TUniquePtr<RHIComputePipelineState>      RHIComputePipelineStatePtr;


class RHI{
public:
	static RHI* Instance();
	static void Initialize();
	static void Release();
	virtual ERHIFormat GetDepthFormat() = 0;
	virtual RHIViewport* GetViewport() = 0;
	virtual void BeginFrame() = 0;
	virtual RHIBufferPtr CreateBuffer(const RHIBufferDesc& desc) = 0;
	virtual RHITexturePtr CreateTexture(const RHITextureDesc& desc) = 0;
	virtual RHISamplerPtr CreateSampler(const RHISamplerDesc& desc) = 0;
	virtual RHIFencePtr CreateFence(bool isSignaled = true) = 0;
	virtual RHIShaderPtr CreateShader(EShaderStageFlagBit type, const char* codeData, size_t codeSize, const XString& entryFunc) = 0;
	virtual RHIGraphicsPipelineStatePtr CreateGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc) = 0;
	virtual RHIComputePipelineStatePtr CreateComputePipelineState(const RHIComputePipelineStateDesc& desc) = 0;
	virtual RHICommandBufferPtr CreateCommandBuffer() = 0;
	// Submit command buffer(s), if bPresent is true, the command buffers will execute after viewport acquired back buffer.
	virtual void SubmitCommandBuffer(RHICommandBuffer* cmd, RHIFence* fence, bool bPresent) = 0;
	virtual void SubmitCommandBuffers(TArrayView<RHICommandBuffer*> cmds, RHIFence* fence, bool bPresent) = 0;
protected:
	friend TUniquePtr<RHI>;
	static TUniquePtr<RHI> s_Instance;
	RHI() = default;
	NON_COPYABLE(RHI);
	NON_MOVEABLE(RHI);
	virtual ~RHI() {}
};