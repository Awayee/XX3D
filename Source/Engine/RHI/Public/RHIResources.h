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
	virtual void UpdateData(const void* data, uint32 byteSize, uint32 offset) = 0;
	XX_NODISCARD const RHIBufferDesc& GetDesc() const { return m_Desc; }
};

// texture
struct RHITextureDesc {
	ETextureDimension Dimension: 8;
	ERHIFormat Format : 8;
	ETextureFlags Flags : 16;
	uint32 Width;
	uint32 Height;
	uint32 Depth; //Only for Texture3D
	uint8 ArraySize;
	uint8 MipSize;
	uint8 Samples;
	uint8 _Padding{ 0 };
	static RHITextureDesc Texture2D();
	static RHITextureDesc TextureCube();
};

struct RHITextureSubDesc {
	uint8 BaseMip{ 0 };
	uint8 MipCount{ 1 };
	uint8 BaseLayer{ 0 };
	uint8 LayerCount{ 1 };
};

struct RHITextureOffset {
	uint8 MipLevel{ 0 };
	uint8 ArrayLayer{ 0 };
	UOffset3D Offset{ 0,0,0 };
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
	virtual void UpdateData(uint32 byteSize, const void* data, RHITextureOffset offset) = 0;
	uint32 GetPixelByteSize();
};

struct RHIRenderTarget {
	RHITexture* m_Texture;
	uint32 m_MipIndex{ 0 };
	uint32 m_ArrayIdx{ 0 };
};

class RHIViewport {
public:
	virtual ~RHIViewport() = default;
	virtual void SetSize(USize2D size) = 0;
	virtual USize2D GetSize() = 0;
	virtual RHITexture* AcquireBackBuffer() = 0;
	virtual RHITexture* GetCurrentBackBuffer() = 0;
	virtual void Present() = 0;
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

// render pass
struct RHIRenderPassInfo {
	struct ColorTargetInfo {
		RHITexture* Target{ nullptr };
		uint32 ArrayIndex{ 0 };
		uint8 MipIndex{ 0 };
		ERTLoadOp LoadOp{ ERTLoadOp::EClear };
		ERTStoreOp StoreOp{ ERTStoreOp::EStore };
		FColor4 ColorClear{ 0.0f, 0.0f, 0.0f, 0.0f };
	};
	struct DepthStencilTargetInfo {
		RHITexture* Target{ nullptr };
		ERTLoadOp DepthLoadOp{ ERTLoadOp::EClear };
		ERTStoreOp DepthStoreOp{ ERTStoreOp::EStore };
		ERTLoadOp StencilLoadOp{ ERTLoadOp::ENoAction };
		ERTStoreOp StencilStoreOp{ ERTStoreOp::ENoAction };
		float DepthClear{ 1.0f };
		uint32 StencilClear{ 0u };
	};
	TStaticArray<ColorTargetInfo, RHI_MAX_RENDER_TARGET_NUM> ColorTargets;
	DepthStencilTargetInfo DepthStencilTarget;
	IURect RenderArea;
	uint32 GetNumColorTargets() const;
};

struct RHIVertexInputInfo {
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
	EBindingType Type;
	EShaderStageFlags StageFlags;
	uint16 Count = 1;
};
typedef TVector<RHIShaderBinding> RHIShaderParamSetLayout;

// pso
struct RHIGraphicsPipelineStateDesc {
	RHIShader* VertexShader;
	RHIShader* PixelShader;
	TVector<RHIShaderParamSetLayout> Layout;
	RHIVertexInputInfo VertexInput;
	RHIBlendDesc BlendDesc;
	RHIRasterizerState RasterizerState;
	RHIDepthStencilState DepthStencilState;
	EPrimitiveTopology PrimitiveTopology;
	TVector<ERHIFormat> ColorFormats;
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
	TVector<RHIShaderParamSetLayout> Layout;
};

class RHIComputePipelineState: public RHIResource {
public:
	RHIComputePipelineState(const RHIComputePipelineStateDesc& desc) : m_Desc(desc) {}
	XX_NODISCARD const RHIComputePipelineStateDesc& GetDesc() const { return m_Desc; }
protected:
	RHIComputePipelineStateDesc m_Desc;
};

struct RHIShaderParam {
	union {
		struct {
			RHIBuffer* Buffer;
			uint32 Offset;
			uint32 Size;
			uint64 _Padding;
		};
		struct {
			RHITexture* Texture;
			RHISampler* Sampler;
			ETextureSRVType SRVType;// optional
			RHITextureSubDesc TextureSub;// valid if SRVType is not Default.
		};
	} Data {nullptr, 0u, 0u, 0ull};
	uint32 ArrayIndex = 0;
	EBindingType Type{ EBindingType::MaxNum };
	static RHIShaderParam UniformBuffer(RHIBuffer* buffer, uint32 offset, uint32 size);
	static RHIShaderParam UniformBuffer(RHIBuffer* buffer) { return UniformBuffer(buffer, 0, buffer->GetDesc().ByteSize); }
	static RHIShaderParam StorageBuffer(RHIBuffer* buffer, uint32 offset, uint32 size);
	static RHIShaderParam Texture(RHITexture* texture, ETextureSRVType srvType = ETextureSRVType::Default, RHITextureSubDesc textureSub = {});
	static RHIShaderParam Sampler(RHISampler* sampler);
	static RHIShaderParam TextureSampler(RHITexture* texture, RHISampler* sampler, RHITextureSubDesc textureSub = {});
};
