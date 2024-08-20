#pragma once
#include "Core/Public/Defines.h"

enum : uint32 {
    RHI_FRAME_IN_FLIGHT_MAX = 1, // TODO optional
    RHI_COLOR_TARGET_MAX = 8,
};

enum class ERHIFormat: uint8 {
    Undefined=0,
    R8_UNORM,
    R8_SNORM,
    R8_UINT,
    R8_SINT,
    R8G8_UNORM,
    R8G8_SNORM,
    R8G8_UINT,
    R8G8_SINT,
    R8G8B8A8_UNORM,
    R8G8B8A8_SNORM,
    R8G8B8A8_UINT,
    R8G8B8A8_SINT,
    R8G8B8A8_SRGB,
    B8G8R8A8_UNORM,
    B8G8R8A8_SRGB,
    R16_UNORM,
    R16_SNORM,
    R16_UINT,
    R16_SINT,
    R16_SFLOAT,
    R16G16_UNORM,
    R16G16_SNORM,
    R16G16_UINT,
    R16G16_SINT,
    R16G16_SFLOAT,
    R16G16B16A16_UNORM,
    R16G16B16A16_SNORM,
    R16G16B16A16_UINT,
    R16G16B16A16_SINT,
    R16G16B16A16_SFLOAT,
    R32_UINT,
    R32_SINT,
    R32_SFLOAT,
    R32G32_UINT,
    R32G32_SINT,
    R32G32_SFLOAT,
    R32G32B32_UINT,
    R32G32B32_SINT,
    R32G32B32_SFLOAT,
    R32G32B32A32_UINT,
    R32G32B32A32_SINT,
    R32G32B32A32_SFLOAT,
    D16_UNORM,
    D24_UNORM_S8_UINT,
    D32_SFLOAT,
    FORMAT_MAX_ENUM
};

enum EBufferFlagBit {
    BUFFER_FLAG_COPY_SRC = 1u,
    BUFFER_FLAG_COPY_DST = 1u << 1,
    BUFFER_FLAG_UNIFORM = 1u << 2,
    BUFFER_FLAG_INDEX = 1u << 3,
    BUFFER_FLAG_VERTEX = 1u << 4,
    BUFFER_FLAG_STORAGE = 1u << 5,
    BUFFER_FLAG_INDIRECT = 1u << 6,
};

typedef uint32 EBufferFlags;

enum class ETextureDimension : uint8 {
    Tex2D=0,
    Tex2DArray,
    TexCube,
    TexCubeArray,
    Tex3D,
    MaxNum,
};

enum ETextureFlagBit {
    TEXTURE_FLAG_COLOR_TARGET = 1u,
    TEXTURE_FLAG_DEPTH_TARGET = 1u << 1,
    TEXTURE_FLAG_STENCIL = 1u << 2,
    TEXTURE_FLAG_SRV = 1u << 3,
    TEXTURE_FLAG_UAV = 1u << 4,
    TEXTURE_FLAG_PRESENT = 1u << 5,
    TEXTURE_FLAG_CPY_SRC = 1u << 6,
    TEXTURE_FLAG_CPY_DST = 1u << 7,
};
typedef uint32 ETextureFlags;

enum class ETextureSRVType : uint8 {
    Default = 0,
    Texture2D,
    CubeMap,
    Depth,
    Stencil,
    MaxNum
};

enum class ESamplerFilter : uint8 {
    Point=0,
    Bilinear,
    Trilinear,
    AnisotropicPoint,
    AnisotropicLinear,
    MaxNum
};

enum class ESamplerAddressMode : uint8 {
    Wrap=0,
    Clamp,
    Mirror,
    Border,
    MaxNum
};

enum class EPrimitiveTopology : uint8 {
    TriangleList,
    TriangleStrip,
    PointList,
    LineList,
    LineStrip,
};

enum class ERTLoadOption : uint8 {
    NoAction,
    Load,
    Clear
};

enum class ERTStoreOption: uint8 {
    ENoAction,
    EStore
};

// shader
enum EShaderStageFlagBit: uint8 {
    SHADER_STAGE_VERTEX_BIT = 1,
    SHADER_STAGE_GEOMETRY_BIT = 2,
    SHADER_STAGE_PIXEL_BIT = 4,
    SHADER_STAGE_COMPUTE_BIT = 8,
};
typedef uint8 EShaderStageFlags;

enum class EBindingType : uint8 {
    Sampler,
    TextureSampler,
    Texture,
    StorageTexture,
    UniformBuffer,
    StorageBuffer,
    MaxNum
};

// blend state
enum class EBlendOption: uint8 {
    Add,
    Sub,
    Min,
    Max,
    ReverseSub,
};

enum class EBlendFactor:uint8 {
    Zero,
    One,
    SrcColor,
    InverseSrcColor,
    DstColor,
    InverseDstColor,
    SrcAlpha,
    InverseSrcAlpha,
    DstAlpha,
    InverseDstAlpha,
    ConstColor,
    InverseConstColor,
    ConstAlpha,
    InverseConstAlpha,
};

enum EColorWriteMaskFlagBit {
    COLOR_WRITE_MASK_R=1,
    COLOR_WRITE_MASK_G=2,
    COLOR_WRITE_MASK_B=4,
    COLOR_WRITE_MASK_A=8,
};
typedef uint32 EColorWriteMaskFlags;

enum class ERasterizerFill : uint8
{
    Point,
    Wireframe,
    Solid,
};

enum class ERasterizerCull: uint8 {
    Back,
    Front,
    Null,
};

enum class ECompareType: uint8 {
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    Equal,
    NotEqual,
    Never,
    Always,
};

enum class EStencilOp: uint8 {
    Keep,
    Zero,
    Replace,
    SaturatedIncrement,
    SaturatedDecrement,
    Invert,
    Increment,
    Decrement,
};

enum class EResourceState : uint8 {
    Unknown,
    ColorTarget,
    DepthStencil,
    ShaderResourceView,
    UnorderedAccessView,
    TransferSrc,
    TransferDst,
    Present
};
