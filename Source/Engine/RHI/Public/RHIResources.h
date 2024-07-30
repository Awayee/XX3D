#pragma once
#include "Core/Public/BaseStructs.h"
#include "Core/Public/TVector.h"
#include "Core/Public/TArray.h"
#include "Core/Public/Log.h"
#include "RHIEnum.h"

class RHIResource {
public:
	RHIResource& operator=(const RHIResource&) = delete;
	virtual void SetName(const char* name) {}
	virtual ~RHIResource() = default;
};

// swapchain

class RHISwapchain {
public:
	virtual ~RHISwapchain() = default;
	virtual void Resize(USize2D size) = 0;
	virtual USize2D GetSize() = 0;
};

// buffer

struct RHIBufferDesc {
	EBufferFlags Flags;
	uint32 ByteSize;
	uint32 Stride;
};

class RHIBuffer: public RHIResource {
protected:
	RHIBufferDesc m_Desc;
public:
	RHIBuffer(const RHIBufferDesc& desc): m_Desc(desc){}
	virtual void UpdateData(const void* data, uint32 byteSize) = 0;
	XX_NODISCARD const RHIBufferDesc& GetDesc() const { return m_Desc; }
};

// texture
struct RHITextureDesc {
	ETextureDimension Dimension;
	ERHIFormat Format;
	ETextureFlags Flags;
	USize3D Size;
	uint32 Depth;
	uint32 ArraySize;
	uint32 MipSize;
	uint32 Samples;
};

struct RHITextureSubDesc {
	uint32 BaseMip;
	uint32 NumMips;
	uint32 BaseLayer;
	uint32 LayerCount;
};

struct RHITextureOffset {
	uint32 MipLevel;
	uint32 ArrayLayer;
	UOffset3D Offset;
};

struct RHITextureCopyRegion {
	RHITextureOffset SrcSub;
	RHITextureOffset DstSub;
	USize3D Extent;
};

class RHITexture: public RHIResource {
protected:
	RHITextureDesc m_Desc;
public:
	RHITexture(const RHITextureDesc& desc) : m_Desc(desc) {}
	XX_NODISCARD const RHITextureDesc& GetDesc() const { return m_Desc; }
	virtual void UpdateData(RHITextureOffset offset, uint32 byteSize, const void* data) = 0;
	uint32 GetPixelByteSize();
};

// sampler
struct RHISamplerDesc {
	ESamplerFilter Filter = ESamplerFilter::Point;
	ESamplerAddressMode AddressU = ESamplerAddressMode::Wrap;
	ESamplerAddressMode AddressV = ESamplerAddressMode::Wrap;
	ESamplerAddressMode AddressW = ESamplerAddressMode::Wrap;
	float LODBias = 0.0f;
	float MinLOD = 0.0f;
	float MaxLOD = FLOAT_MAX;
	float MaxAnisotropy = 0.0f;
};

class RHISampler: public RHIResource {
protected:
	RHISamplerDesc m_Desc;
public:
	RHISampler(const RHISamplerDesc& desc): m_Desc(desc){}
	XX_NODISCARD const RHISamplerDesc& GetDesc() const { return m_Desc; }
};

struct RHIRenderTargetView {
	RHITexture* m_Texture;
	uint32 m_Miplevel;
	uint32 m_ArrayIdx;
};

// fence
class RHIFence: public RHIResource {
public:
	virtual ~RHIFence() = default;
	virtual void Wait() = 0;
	virtual void Reset() = 0;
};

class RHIShader : public RHIResource {
public:
	RHIShader(EShaderStageFlagBit type) : m_Type(type) {}
	virtual ~RHIShader() {}
	XX_NODISCARD EShaderStageFlagBit GetStage() const { return m_Type; }
protected:
	EShaderStageFlagBit m_Type;
};

static constexpr uint32 MAX_RENDER_TARGET_NUM = 8;

// render pass
struct RHIRenderPassDesc {
	struct ColorTargetInfo {
		RHITexture* Target;
		uint32 ArrayIndex;
		uint8 MipIndex;
		ERTLoadOp LoadOp{ ERTLoadOp::EClear };
		ERTStoreOp StoreOp{ ERTStoreOp::EStore };
		FColor4 ColorClear{ 0.0f, 0.0f, 0.0f, 0.0f };
	};
	struct DepthStencilTargetInfo {
		RHITexture* Target;
		ERTLoadOp DepthLoadOp{ ERTLoadOp::EClear };
		ERTStoreOp DepthStoreOp{ ERTStoreOp::EStore };
		ERTLoadOp StencilLoadOp{ ERTLoadOp::ENoAction };
		ERTStoreOp StencilStoreOp{ ERTStoreOp::ENoAction };
		float DepthClear{ 1.0f };
		uint32 StencilClear{ 0u };
	};
	TStaticArray<ColorTargetInfo, MAX_RENDER_TARGET_NUM> ColorTargets;
	DepthStencilTargetInfo DepthStencilTarget;
	IURect RenderArea;
};

class RHIRenderPass : public RHIResource {
public:
	RHIRenderPass(const RHIRenderPassDesc& desc) : m_Desc(desc){}
	virtual ~RHIRenderPass() {}
	const RHIRenderPassDesc& GetDesc() { return m_Desc; }
protected:
	RHIRenderPassDesc m_Desc;
};

struct RHIVertexInput {
	struct BindingDesc {
		uint32 Binding;
		uint32 Stride;
		bool PerInstance;// per instance or per vertex
	};
	struct AttributeDesc {
		uint32 Location;
		uint32 Binding;
		ERHIFormat Format;
		uint32 Offset;
	};
	TVector<BindingDesc> Bindings;
	TVector<AttributeDesc> Attributes;
};

struct RHIBlendState {
	bool Enable;
	EBlendOption ColorBlendOp;
	EBlendFactor ColorSrc;
	EBlendFactor ColorDst;
	EBlendOption AlphaBlendOp;
	EBlendFactor AlphaSrc;
	EBlendFactor AlphaDst;
	EColorWriteMaskFlags ColorWriteMask;
};

struct RHIBlendDesc {
	TVector<RHIBlendState> BlendStates;
	float BlendConst[4];
};

struct RHIRasterizerState {
	ERasterizerFill FillMode;
	ERasterizerCull CullMode;
	bool Clockwise{ false };
	float DepthBias = 0.0f;
	float SlopeScaleDepthBias = 0.0f;
};

struct RHIDepthStencilState {
	struct StencilOpDesc {
		EStencilOp FailOp;
		EStencilOp PassOp;
		EStencilOp DepthFailOp;
		ECompareType CompareType;
		uint8 ReadMask;
		uint8 WriteMask;
	};
	bool DepthWrite;
	ECompareType DepthCompare;
	bool StencilTest;
	StencilOpDesc FrontStencil;
	StencilOpDesc BackStencil;
};

// layout
struct RHIShaderBinding {
	uint32 Count;
	EBindingType Type;
	EShaderStageFlags StageFlags;
};
typedef TVector<RHIShaderBinding> RHIShaderParemeterLayout;
typedef TVector<RHIShaderParemeterLayout> RHIPipelineLayout;

// pso
struct RHIGraphicsPipelineStateDesc {
	RHIShader* VertexShader;
	RHIShader* PixelShader;
	RHIPipelineLayout Layout;
	RHIVertexInput VertexInput;
	RHIBlendDesc BlendDesc;
	RHIRasterizerState RasterizerState;
	RHIDepthStencilState DepthStencilState;
	EPrimitiveTopology PrimitiveTopology;
	TVector<ERHIFormat> RenderTargetFormats;
	ERHIFormat DepthStencilFormat;
	uint8 NumSamples;
};

class RHIGraphicsPipelineState: public RHIResource {
public:
	RHIGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc) : m_Desc(desc) {}
	XX_NODISCARD const RHIGraphicsPipelineStateDesc& GetDesc() const { return m_Desc; }
protected:
	RHIGraphicsPipelineStateDesc m_Desc;
};

// compute pipeline
struct RHIComputePipelineStateDesc {
	RHIShader* Shader;
	RHIPipelineLayout Layout;
};

class RHIComputePipelineState: public RHIResource {
public:
	RHIComputePipelineState(const RHIComputePipelineStateDesc& desc) : m_Desc(desc) {}
	XX_NODISCARD const RHIComputePipelineStateDesc& GetDesc() const { return m_Desc; }
protected:
	RHIComputePipelineStateDesc m_Desc;
};

class RHIShaderParameterSet {
public:
	virtual ~RHIShaderParameterSet(){}
	virtual void SetUniformBuffer(uint32 binding, RHIBuffer* buffer) = 0;
	virtual void SetStorageBuffer(uint32 binding, RHIBuffer* buffer) = 0;
	virtual void SetTexture(uint32 binding, RHITexture* image) = 0;
	virtual void SetSampler(uint32 binding, RHISampler* sampler) = 0;
};

