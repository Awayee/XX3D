#pragma once
#include "RHI/Public/RHI.h"
#include "D3D12Util.h"

class D3D12Device;
class D3D12Viewport;

class D3D12RHI final: RHI {
public:
	explicit D3D12RHI(const RHIInitDesc& desc);
	~D3D12RHI() override;
	void BeginFrame() override;
	ERHIFormat GetDepthFormat() override;
	RHIViewport* GetViewport() override;
	RHIBufferPtr CreateBuffer(const RHIBufferDesc& desc) override;
	RHITexturePtr CreateTexture(const RHITextureDesc& desc) override;
	RHISamplerPtr CreateSampler(const RHISamplerDesc& desc) override;
	RHIFencePtr CreateFence(bool isSignaled) override;
	RHIShaderPtr CreateShader(EShaderStageFlags type, const char* codeData, uint32 codeSize, const XString& entryFunc) override;
	RHIGraphicsPipelineStatePtr CreateGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc) override;
	RHIComputePipelineStatePtr CreateComputePipelineState(const RHIComputePipelineStateDesc& desc) override;
	RHICommandBufferPtr CreateCommandBuffer(EQueueType queue) override;
	void SubmitCommandBuffers(TArrayView<RHICommandBuffer*> cmds, EQueueType queue, RHIFence* fence, bool bPresent) override;
	D3D12Device* GetDevice();
private:
	TDXPtr<IDXGIFactory4> m_DXGIFactory;
	TUniquePtr<D3D12Device> m_Device;
	TUniquePtr<D3D12Viewport> m_Viewport;
	ERHIFormat m_DepthFormat{ERHIFormat::D24_UNORM_S8_UINT };

	bool CreateDeviceContext();
};

#define DX_DEVICE  D3D12RHI::InstanceD3D12()->Device()
