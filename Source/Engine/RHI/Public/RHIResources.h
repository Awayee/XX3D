#pragma once
#include "Core/Public/BaseStructs.h"
#include "Core/Public/TArray.h"
#include "Core/Public/Log.h"
#include "RHIEnum.h"

class RHIResource {
public:
	RHIResource& operator=(const RHIResource&) = delete;
	virtual void SetName(const char* name) {}
	virtual ~RHIResource() = default;
};

uint32 GetRHIFormatPixelSize(ERHIFormat format);

uint16 GetTextureDimension2DSize(ETextureDimension dimension);
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
	uint32 Stride {0}; // for vertex buffer
	ERHIFormat IndexFormat{ ERHIFormat::R32_UINT }; // for index buffer
};

class RHIBuffer: public RHIResource {
protected:
	RHIBufferDesc m_Desc;
public:
	RHIBuffer(const RHIBufferDesc& desc): m_Desc(desc){}
	virtual void UpdateData(const void* data, uint32 byteSize, uint32 offset) = 0;
	XX_NODISCARD const RHIBufferDesc& GetDesc() const { return m_Desc; }
};

struct RHIDynamicBuffer {
	uint32 BufferIndex;
	uint32 Offset;
	uint32 Size;
	uint32 Stride;
	bool operator==(const RHIDynamicBuffer& rhs) const;
};

// texture sub resource
struct RHITextureSubRes {
	uint16 ArrayIndex{ 0 };
	uint16 ArraySize{ 1 };
	uint8 MipIndex{ 0 };
	uint8 MipSize{ 1 };
	ETextureDimension Dimension{ ETextureDimension::Tex2D };
	ETextureViewFlags ViewFlags{ ETextureViewFlags::Color };
	bool operator ==(const RHITextureSubRes& rhs) const;
	static RHITextureSubRes Tex2D(uint16 arrayIndex, ETextureViewFlags viewFlags);
};

// texture
struct RHITextureDesc {
	ETextureDimension Dimension: 8;
	ERHIFormat Format : 8;
	ETextureFlags Flags : 16;
	uint32 Width;
	uint32 Height;
	uint32 Depth; //Only for Texture3D
	uint16 ArraySize;
	uint8 MipSize;
	uint8 Samples;
	union {
		FColor4 Color;
		struct {
			float Depth;
			uint8 Stencil;
		}DepthStencil;
	} ClearValue;
	uint32 GetPixelByteSize() const;
	RHITextureSubRes GetDefaultSubRes() const;
	RHITextureSubRes GetSubRes2D(uint16 arrayIndex, ETextureViewFlags viewFlags) const;
	RHITextureSubRes GetSubRes2DArray(ETextureViewFlags viewFlags) const;
	uint32 GetMipLevelWidth(uint8 mipLevel) const;
	uint32 GetMipLevelHeight(uint8 mipLevel) const;
	uint16 Get2DArraySize() const;
	void LimitSubRes(RHITextureSubRes& subRes) const; // correct the sub resource to limit its range.
	bool IsAllSubRes(RHITextureSubRes subRes) const; // check sub resource is all resource
	static RHITextureDesc Texture2D();
	static RHITextureDesc TextureCube();
	static RHITextureDesc Texture2DArray(uint8 arraySize);
};

struct RHITextureCopyRegion {
	RHITextureSubRes SrcSubRes;// MipSize of SubRes is useless
	RHITextureSubRes DstSubRes;
	UOffset3D SrcOffset;
	UOffset3D DstOffset;
	USize3D Extent;
};

class RHITexture: public RHIResource {
protected:
	RHITextureDesc m_Desc;
public:
	RHITexture(const RHITextureDesc& desc) : m_Desc(desc) {}
	XX_NODISCARD const RHITextureDesc& GetDesc() const { return m_Desc; }
	virtual void UpdateData(const void* data, uint32 byteSize, RHITextureSubRes subRes, IOffset3D offset) = 0;
	void UpdateData(uint32 byteSize, const void* data);
	static bool IsDimensionCompatible(ETextureDimension baseDimension, ETextureDimension targetDimension);
};

class RHIViewport {
public:
	virtual ~RHIViewport() = default;
	virtual void SetSize(USize2D size) = 0;
	virtual USize2D GetSize() = 0;
	virtual bool PrepareBackBuffer() = 0;
	virtual RHITexture* GetBackBuffer() = 0;
	virtual ERHIFormat GetBackBufferFormat() = 0;
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
	virtual void Wait() = 0; // wait until signaled
	virtual void Reset() = 0; // reset the fence to unsignaled state
};

class RHIShader : public RHIResource {
public:
	RHIShader(EShaderStageFlags type) : m_Type(type) {}
	virtual ~RHIShader() {}
	XX_NODISCARD EShaderStageFlags GetStage() const { return m_Type; }
protected:
	EShaderStageFlags m_Type;
};

// render pass
struct RHIRenderPassInfo {
	struct ColorTargetInfo {
		RHITexture* Target{ nullptr };
		uint16 ArrayIndex{ 0 };
		uint8 MipIndex{ 0 };
		ERTLoadOption LoadOp{ ERTLoadOption::Clear };
		ERTStoreOption StoreOp{ ERTStoreOption::EStore };
		FColor4 ColorClear{ 0.0f, 0.0f, 0.0f, 0.0f };
	};
	struct DepthStencilTargetInfo {
		RHITexture* Target{ nullptr };
		uint16 ArrayIndex{ 0 };
		uint8 MipIndex{ 0 };
		ERTLoadOption DepthLoadOp{ ERTLoadOption::Clear };
		ERTStoreOption DepthStoreOp{ ERTStoreOption::EStore };
		ERTLoadOption StencilLoadOp{ ERTLoadOption::NoAction };
		ERTStoreOption StencilStoreOp{ ERTStoreOption::ENoAction };
		float DepthClear{ 1.0f };
		uint8 StencilClear{ 0u };
	};
	TStaticArray<ColorTargetInfo, RHI_COLOR_TARGET_MAX> ColorTargets;
	DepthStencilTargetInfo DepthStencilTarget;
	Rect RenderArea;
	uint32 GetNumColorTargets() const;
};

struct RHIVertexInputInfo {
	struct BindingDesc {
		uint32 Binding;
		uint32 Stride;
		bool PerInstance;// per instance or per vertex
	};
	struct AttributeDesc {
		const char* SemanticName;
		uint32 SemanticIndex;
		uint32 Location;
		uint32 Binding;
		ERHIFormat Format;
		uint32 Offset;
	};
	TArray<BindingDesc> Bindings;
	TArray<AttributeDesc> Attributes;
};

// Semantic defines
#define POSITION(idx) "POSITION", idx
#define COLOR(idx) "COLOR", idx
#define NORMAL(idx) "NORMAL", idx
#define TEXCOORD(idx) "TEXCOORD", idx
#define TANGENT(idx) "TANGENT", idx
#define BINORMAL(idx) "BINORMAL", idx
#define BLENDINDICES(idx) "BLENDINDICES", idx
#define BLENDWEIGHT(idx) "BLENDWEIGHT", idx
#define INSTANCE_TRANSFORM(idx) "INSTANCE_TRANSFORM", idx

struct RHIBlendState {
	bool Enable{ false };
	EBlendOption ColorBlendOp{ EBlendOption::Add };
	EBlendFactor ColorSrc{ EBlendFactor::One };
	EBlendFactor ColorDst{ EBlendFactor::Zero };
	EBlendOption AlphaBlendOp{ EBlendOption::Add };
	EBlendFactor AlphaSrc{ EBlendFactor::One };
	EBlendFactor AlphaDst{ EBlendFactor::Zero };
	EColorComponentFlags ColorWriteMask {EColorComponentFlags::All};
};

struct RHIBlendDesc {
	TStaticArray<RHIBlendState, RHI_COLOR_TARGET_MAX> BlendStates;
	bool LogicOpEnable;
	ELogicOp LogicOp;
	float BlendConst[4];
};

struct RHIRasterizerState {
	ERasterizerFill FillMode{ ERasterizerFill::Solid };
	ERasterizerCull CullMode{ ERasterizerCull::Back };
	bool FrontClockwise{ false };
	float DepthBiasConstant = 0.0f;
	float DepthBiasSlope = 0.0f;
	float DepthBiasClamp = 0.0f;
};

struct RHIDepthStencilState {
	struct StencilOpDesc {
		EStencilOp FailOp;
		EStencilOp PassOp;
		EStencilOp DepthFailOp;
		ECompareType CompareType;
	};
	bool DepthTest;
	bool DepthWrite;
	ECompareType DepthCompare;
	bool StencilTest;
	uint8 StencilReadMask;
	uint8 StencilWriteMask;
	StencilOpDesc FrontStencil;
	StencilOpDesc BackStencil;
};

// layout
struct RHIShaderBinding {
	EBindingType Type;
	EShaderStageFlags StageFlags;
	uint16 Count = 1;
};

typedef TArray<RHIShaderBinding> RHIShaderParamSetLayout;

// pso
struct RHIGraphicsPipelineStateDesc {
	RHIShader* VertexShader;
	RHIShader* PixelShader;
	TArray<RHIShaderParamSetLayout> Layout;
	RHIVertexInputInfo VertexInput;
	RHIBlendDesc BlendDesc;
	RHIRasterizerState RasterizerState;
	RHIDepthStencilState DepthStencilState;
	EPrimitiveTopology PrimitiveTopology{ EPrimitiveTopology::TriangleList };
	TStaticArray<ERHIFormat, RHI_COLOR_TARGET_MAX> ColorFormats{ ERHIFormat::Undefined };
	uint8 NumColorTargets{ 0 };
	ERHIFormat DepthStencilFormat{ ERHIFormat::Undefined };
	uint8 NumSamples{ 1 };
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
	TArray<RHIShaderParamSetLayout> Layout;
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
		};
		struct {
			RHITexture* Texture;
			RHITextureSubRes SubRes;// valid if SRVType is not Default.
		};
		struct {
			RHISampler* Sampler;
			uint64 Padding;
		};
		RHIDynamicBuffer DynamicBuffer;
	} Data {nullptr, 0u, 0u};
	uint32 ArrayIndex = 0;
	EBindingType Type{ EBindingType::MaxNum };
	bool IsDynamicBuffer{ false };
	bool operator==(const RHIShaderParam& rhs) const;
	bool operator!=(const RHIShaderParam& rhs) const;
	static RHIShaderParam UniformBuffer(RHIBuffer* buffer, uint32 offset, uint32 size);
	static RHIShaderParam UniformBuffer(RHIBuffer* buffer) { return UniformBuffer(buffer, 0, buffer->GetDesc().ByteSize); }
	static RHIShaderParam StorageBuffer(RHIBuffer* buffer, uint32 offset, uint32 size);
	static RHIShaderParam UniformBuffer(const RHIDynamicBuffer& dynamicBuffer);
	static RHIShaderParam StorageBuffer(const RHIDynamicBuffer& dynamicBuffer);
	static RHIShaderParam Texture(RHITexture* texture, RHITextureSubRes textureSub);
	static RHIShaderParam Texture(RHITexture* texture);
	static RHIShaderParam Sampler(RHISampler* sampler);
};
