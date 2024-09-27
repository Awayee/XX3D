#pragma once
#include "RHI/Public/RHICommand.h"
#include "Core/Public/TArrayView.h"
#include "Core/Public/String.h"
#include "Core/Public/TUniquePtr.h"

#ifdef _DEBUG
#define ENABLE_RHI_DEBUG true
#else
#define ENABLE_RHI_DEBUG false
#endif

// type defines
typedef TUniquePtr<RHICommandBuffer>             RHICommandBufferPtr;
typedef TUniquePtr<RHIBuffer>                    RHIBufferPtr;
typedef TUniquePtr<RHITexture>                   RHITexturePtr;
typedef TUniquePtr<RHISampler>                   RHISamplerPtr;
typedef TUniquePtr<RHIFence>                     RHIFencePtr;
typedef TUniquePtr<RHIShader>                    RHIShaderPtr;
typedef TUniquePtr<RHIGraphicsPipelineState>     RHIGraphicsPipelineStatePtr;
typedef TUniquePtr<RHIComputePipelineState>      RHIComputePipelineStatePtr;
typedef void* WindowHandle;

struct RHIInitConfig {
	bool EnableDebug : 1;
	bool EnableVSync : 1;
	bool EnableMSAA  : 1;
	RHIInitConfig() : EnableDebug(ENABLE_RHI_DEBUG), EnableVSync(false), EnableMSAA(false) {}
};
typedef RHIInitConfig(*RHIInitConfigBuilder)();

class RHI{
public:
	static void SetInitConfigBuilder(RHIInitConfigBuilder f);
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
	virtual RHIShaderPtr CreateShader(EShaderStageFlags type, const char* codeData, uint32 codeSize, const XString& entryFunc) = 0;
	virtual RHIGraphicsPipelineStatePtr CreateGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc) = 0;
	virtual RHIComputePipelineStatePtr CreateComputePipelineState(const RHIComputePipelineStateDesc& desc) = 0;
	virtual RHICommandBufferPtr CreateCommandBuffer(EQueueType queue) = 0;
	// Submit command buffer(s), multi command buffers in one call will execute in parallel.
	// if bPresent is true, the command buffers will execute after viewport acquired back buffer.
	virtual void SubmitCommandBuffers(TArrayView<RHICommandBuffer*> cmds, EQueueType queue, RHIFence* fence, bool bPresent) = 0;

	void CaptureFrame();
protected:
	friend TDefaultDeleter<RHI>;
	static TUniquePtr<RHI> s_Instance;
	static RHIInitConfigBuilder s_InitConfigBuilder;
	RHI() = default;
	NON_COPYABLE(RHI);
	NON_MOVEABLE(RHI);
	virtual ~RHI() = default;
};