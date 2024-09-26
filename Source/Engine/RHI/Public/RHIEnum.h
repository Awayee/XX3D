#pragma once
#include "Core/Public/Defines.h"
#include "Core/Public/EnumClass.h"

enum : uint32 {
    RHI_FRAME_IN_FLIGHT_MAX = 1,// TODO solve the errors when RHI_FRAME_IN_FLIGHT_MAX greater than 1
    RHI_COLOR_TARGET_MAX = 8,
    RHI_TEXTURE_ARRAY_MAX = 1024,
    RHI_TEXTURE_MIP_MAX = 64,
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

enum class EBufferFlags {
    CopySrc = 1u,
    CopyDst = 1u << 1,
    Uniform = 1u << 2,
    Index = 1u << 3,
    Vertex = 1u << 4,
    Storage = 1u << 5,
    IndirectDraw = 1u << 6,
    Readback = 1u << 7
};
ENUM_CLASS_FLAGS(EBufferFlags);

enum class ETextureDimension : uint8 {
    Tex2D=0,
    Tex2DArray,
    TexCube,
    TexCubeArray,
    Tex3D,
    MaxNum,
};

enum class ETextureViewFlags : uint8 {
    Color = 1,
    Depth = 1 << 1,
    Stencil = 1 << 2,
    MetaData = 1 << 3,
    DepthStencil = Depth|Stencil,
};
ENUM_CLASS_FLAGS(ETextureViewFlags);

enum class ETextureFlags {
    ColorTarget = 1u,
    DepthStencilTarget = 1u << 1,
    SRV = 1u << 2,
    UAV = 1u << 3,
    Present = 1u << 4,
    CopySrc = 1u << 5,
    CopyDst = 1u << 6,
};
ENUM_CLASS_FLAGS(ETextureFlags);

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
enum class EShaderStageFlags: uint8 {
    Vertex = 1,
    Geometry = 2,
    Pixel = 4,
    Compute = 8,
};
ENUM_CLASS_FLAGS(EShaderStageFlags);

enum class EBindingType : uint8 {
    Sampler,
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

enum class EColorComponentFlags {
    R = 1,
    G = 2,
    B = 4,
    A = 8,
    All = R | G | B | A,
};
ENUM_CLASS_FLAGS(EColorComponentFlags);

enum class ERasterizerFill : uint8{
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

enum class EQueueType : uint8 {
    Graphics = 0,
    Compute,
    Transfer,
    Count
};

enum class ELogicOp {
    Clear = 0,
    And = 1,
    AndReverse = 2,
    Copy = 3,
    AndInverted = 4,
    NoOp = 5,
    XOr = 6,
    Or = 7,
    NOr = 8,
    Equivalent = 9,
    Invert = 10,
    OrReverse = 11,
    CopyInverted = 12,
    OrInverted = 13,
    NAnd = 14,
    Set = 15,
};