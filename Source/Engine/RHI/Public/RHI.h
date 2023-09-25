#pragma once
#include "RHIClasses.h"
#include "RHIResources.h"
#include "Core/Public/TArrayView.h"
#include "Core/Public/Func.h"
#include "Core/Public/String.h"

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
	virtual RRenderPass* CreateRenderPass(TConstArrayView<RSAttachment> attachments, TConstArrayView<RSubPassInfo> subpasses, TConstArrayView<RSubpassDependency> dependencies) = 0;
	// for single subpass
	virtual RRenderPass* CreateRenderPass(TConstArrayView<RSAttachment> colorAttachments, const RSAttachment* depthAttachment)=0;
	virtual void DestroyRenderPass(RRenderPass* pass) = 0;

	// descriptor set
	virtual RDescriptorSetLayout* CreateDescriptorSetLayout(TConstArrayView<RSDescriptorSetLayoutBinding> bindings) = 0;
	virtual void DestroyDescriptorSetLayout(RDescriptorSetLayout* descriptorSetLayout) = 0;
	virtual RDescriptorSet* AllocateDescriptorSet(const RDescriptorSetLayout* layout) = 0;
	virtual void FreeDescriptorSet(RDescriptorSet* descriptorSet) = 0;
	//virtual void AllocateDescriptorSets(uint32 count, const RDescriptorSetLayout* const* layouts, RDescriptorSet*const* descriptorSets) = 0;
	//virtual void FreeDescriptorSets(uint32 count, RDescriptorSet** descriptorSets) = 0;

	// cmd
	virtual RHICommandBuffer* AllocateCommandBuffer(RCommandBufferLevel level) = 0;
	virtual void SubmitCommandBuffer(const RHICommandBuffer* cmd, RHIFence* fence, RHISwapChain* swapChain) = 0;
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
