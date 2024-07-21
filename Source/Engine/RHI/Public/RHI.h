#pragma once
#include "RHI/Public/RHICommand.h"
#include "Core/Public/TArrayView.h"
#include "Core/Public/String.h"
#include "Core/Public/TUniquePtr.h"

typedef void(*DebugFunc)(const char*);

struct RHIInitDesc {
	XXString AppName;
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
typedef TUniquePtr<RHIRenderPass>                RHIRenderPassPtr;
typedef TUniquePtr<RHIShaderParameterSet>        RHIShaderParameterSetPtr;


class RHI{
public:
	static RHI* Instance();
	static void Initialize();
	static void Release();
	virtual ERHIFormat GetDepthFormat() = 0;
	virtual USize2D GetRenderArea() = 0;

	virtual RHIBufferPtr CreateBuffer(const RHIBufferDesc& desc, void* defaultData) = 0;
	virtual RHITexturePtr CreateTexture(const RHITextureDesc& desc, void* defaultData) = 0;
	virtual RHISamplerPtr CreateSampler(const RHISamplerDesc& desc) = 0;
	virtual RHIFencePtr CreateFence(bool sig = true) = 0;
	virtual RHIShaderPtr CreateShader(EShaderStageFlagBit type, const char* codeData, size_t codeSize, const char* entryFunc) = 0;
	virtual RHIGraphicsPipelineStatePtr CreateGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc) = 0;
	virtual RHIComputePipelineStatePtr CreateComputePipelineState(const RHIComputePipelineStateDesc& desc) = 0;
	virtual RHIRenderPassPtr CreateRenderPass(const RHIRenderPassDesc& desc) = 0;
	virtual RHIShaderParameterSetPtr CreateShaderParameterSet(const RHIShaderParemeterLayout& layout) = 0;

	virtual RHICommandBufferPtr CreateCommandBuffer() = 0;
	virtual void SubmitCommandBuffer(RHICommandBuffer* cmd, RHIFence* fence) = 0;
	virtual void SubmitCommandBuffer(TArrayView<RHICommandBuffer*> cmds, RHIFence* fence) = 0;
	virtual void Update() = 0;
	virtual void Present() = 0;

	RHI() = default;
	RHI(const RHI&) = delete;
	RHI(RHI&&) = delete;
	RHI* operator=(const RHI&) = delete;
	virtual ~RHI() {}
private:
	static TUniquePtr<RHI> s_Instance;
};