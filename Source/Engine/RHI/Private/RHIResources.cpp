#include "RHI/Public/RHIResources.h"
#include "Math/Public/Math.h"


uint32 GetRHIFormatPixelSize(ERHIFormat format) {
    struct FormatTexelInfo {
        ERHIFormat Format;
        uint32 PixelByteSize;
    };
    static FormatTexelInfo g_FormatPixelByteSize[EnumCast(ERHIFormat::FORMAT_MAX_ENUM)] = {
         {ERHIFormat::Undefined,            0             },
         {ERHIFormat::R8_UNORM,             1             },
         {ERHIFormat::R8_SNORM,             1             },
         {ERHIFormat::R8_UINT,              1             },
         {ERHIFormat::R8_SINT,              1             },
         {ERHIFormat::R8G8_UNORM,           2             },
         {ERHIFormat::R8G8_SNORM,           2             },
         {ERHIFormat::R8G8_UINT,            2             },
         {ERHIFormat::R8G8_SINT,            2             },
         {ERHIFormat::R8G8B8A8_UNORM,       4             },
         {ERHIFormat::R8G8B8A8_SNORM,       4             },
         {ERHIFormat::R8G8B8A8_UINT,        4             },
         {ERHIFormat::R8G8B8A8_SINT,        4             },
         {ERHIFormat::R8G8B8A8_SRGB,        4             },
         {ERHIFormat::R16_UNORM,            2             },
         {ERHIFormat::R16_SNORM,            2             },
         {ERHIFormat::R16_UINT,             2             },
         {ERHIFormat::R16_SINT,             2             },
         {ERHIFormat::R16_SFLOAT,           4             },
         {ERHIFormat::R16G16_UNORM,         4             },
         {ERHIFormat::R16G16_SNORM,         4             },
         {ERHIFormat::R16G16_UINT,          4             },
         {ERHIFormat::R16G16_SINT,          4             },
         {ERHIFormat::R16G16_SFLOAT,        4             },
         {ERHIFormat::R16G16B16A16_UNORM,   8             },
         {ERHIFormat::R16G16B16A16_SNORM,   8             },
         {ERHIFormat::R16G16B16A16_UINT,    8             },
         {ERHIFormat::R16G16B16A16_SINT,    8             },
         {ERHIFormat::R16G16B16A16_SFLOAT,  8             },
         {ERHIFormat::R32_UINT,             4             },
         {ERHIFormat::R32_SINT,             4             },
         {ERHIFormat::R32_SFLOAT,           4             },
         {ERHIFormat::R32G32_UINT,          8             },
         {ERHIFormat::R32G32_SINT,          8             },
         {ERHIFormat::R32G32_SFLOAT,        8             },
         {ERHIFormat::R32G32B32_UINT,       12            },
         {ERHIFormat::R32G32B32_SINT,       12            },
         {ERHIFormat::R32G32B32_SFLOAT,     12            },
         {ERHIFormat::R32G32B32A32_UINT,    16            },
         {ERHIFormat::R32G32B32A32_SINT,    16            },
         {ERHIFormat::R32G32B32A32_SFLOAT,  16            },
         {ERHIFormat::D16_UNORM,            2             },
         {ERHIFormat::D24_UNORM_S8_UINT,    4             },
         {ERHIFormat::D32_SFLOAT,           4             },
    };
    return g_FormatPixelByteSize[EnumCast(format)].PixelByteSize;
}

uint16 GetTextureDimension2DSize(ETextureDimension dimension) {
    switch (dimension) {
    case ETextureDimension::Tex2D:
    case ETextureDimension::Tex3D:
    case ETextureDimension::Tex2DArray: return 1;
    case ETextureDimension::TexCube:
    case ETextureDimension::TexCubeArray: return 6;
    default:
        return 1;
    }
}

uint32 RHITextureDesc::GetPixelByteSize() const {
    return GetRHIFormatPixelSize(Format);
}

RHITextureSubRes RHITextureDesc::GetDefaultSubRes() const {
    ETextureViewFlags defaultView;
    if (EnumHasAnyFlags(Flags, ETextureFlags::DepthStencilTarget)) {
        defaultView = ETextureViewFlags::DepthStencil;
    }
    else {
        defaultView = ETextureViewFlags::Color;
    }
    return { 0, ArraySize, 0, MipSize, Dimension, defaultView };
}

RHITextureSubRes RHITextureDesc::GetSubRes2D(uint16 arrayIndex, ETextureViewFlags viewFlags) const {
    CHECK(arrayIndex < ArraySize);
    return RHITextureSubRes{ arrayIndex, 1, 0, MipSize, ETextureDimension::Tex2D, viewFlags };
}

RHITextureSubRes RHITextureDesc::GetSubRes2DArray(ETextureViewFlags viewFlags) const {
    return RHITextureSubRes{ 0, ArraySize, 0, MipSize, ETextureDimension::Tex2DArray, viewFlags };
}

uint32 RHITextureDesc::GetMipLevelWidth(uint8 mipLevel) const {
    return Math::Max<uint32>(1u, Width >> mipLevel);
}

uint32 RHITextureDesc::GetMipLevelHeight(uint8 mipLevel) const {
    return Math::Max<uint32>(1u, Height >> mipLevel);
}

uint16 RHITextureDesc::Get2DArraySize() const {
    return (uint32)ArraySize * GetTextureDimension2DSize(Dimension);
}

inline uint16 ConvertDimensionArraySize(uint16 arraySize, ETextureDimension srcDimension, ETextureDimension dstDimension) {
    const uint16 srcDimension2DSize = GetTextureDimension2DSize(srcDimension);
    const uint16 dstDimension2DSize = GetTextureDimension2DSize(dstDimension);
    return arraySize * srcDimension2DSize / dstDimension2DSize;
}

void RHITextureDesc::LimitSubRes(RHITextureSubRes& subRes) const {
    // limit mip
    subRes.MipIndex = Math::Min<decltype(MipSize)>(subRes.MipIndex, MipSize - 1);
    subRes.MipSize = Math::Min<decltype(MipSize)>(subRes.MipSize, MipSize - subRes.MipIndex);
    // limit array size
    const ETextureDimension subDimension = subRes.Dimension;
    const uint16 subMaxArraySize = ConvertDimensionArraySize(ArraySize, Dimension, subDimension);
    subRes.ArrayIndex = Math::Min<uint16>(subMaxArraySize-1, subRes.ArrayIndex);
    if(ETextureDimension::Tex2D == subDimension) {
        subRes.ArraySize = 1;
    }
    if(ETextureDimension::TexCube == subDimension) {
        subRes.ArraySize = 1;
    }
    else {
        subRes.ArraySize = Math::Min<uint16>(subMaxArraySize-subRes.ArrayIndex, subRes.ArraySize);
    }
}

bool RHITextureDesc::IsAllSubRes(RHITextureSubRes subRes) const {
    return subRes.ArrayIndex == 0 && subRes.ArraySize == ArraySize &&
        subRes.MipIndex == 0 && subRes.MipSize == MipSize;
}

RHITextureDesc RHITextureDesc::Texture2D() {
    RHITextureDesc desc{};
    desc.Dimension = ETextureDimension::Tex2D;
    desc.Depth = 1;
    desc.ArraySize = 1;
    desc.MipSize = 1;
    desc.Samples = 1;
    desc.ClearValue.Color = { 0.0f, 0.0f, 0.0f, 0.0f };
    return desc;
}

RHITextureDesc RHITextureDesc::TextureCube() {
    RHITextureDesc desc{};
    desc.Dimension = ETextureDimension::TexCube;
    desc.Depth = 1;
    desc.ArraySize = 1;
    desc.MipSize = 1;
    desc.Samples = 1;
    desc.ClearValue.Color = { 0.0f, 0.0f, 0.0f, 0.0f };
    return desc;
}

RHITextureDesc RHITextureDesc::Texture2DArray(uint8 arraySize) {
    RHITextureDesc desc{};
    desc.Dimension = ETextureDimension::Tex2DArray;
    desc.Depth = 1;
    desc.ArraySize = arraySize;
    desc.MipSize = 1;
    desc.Samples = 1;
    desc.ClearValue.Color = { 0.0f, 0.0f, 0.0f, 0.0f };
    return desc;
}

void RHITexture::UpdateData(uint32 byteSize, const void* data) {
    UpdateData(data, byteSize, m_Desc.GetDefaultSubRes(), { 0,0,0 });
}

bool RHITexture::IsDimensionCompatible(ETextureDimension baseDimension, ETextureDimension targetDimension) {
    if (baseDimension == ETextureDimension::Tex2D) {
        return targetDimension == ETextureDimension::Tex2D;
    }
    if (baseDimension == ETextureDimension::Tex2DArray) {
        return targetDimension == ETextureDimension::Tex2DArray || targetDimension == ETextureDimension::Tex2D;
    }
    if (baseDimension == ETextureDimension::TexCube) {
        return targetDimension == ETextureDimension::Tex2D || targetDimension == ETextureDimension::TexCube;
    }
    if (baseDimension == ETextureDimension::TexCubeArray) {
        return targetDimension == ETextureDimension::Tex2D || targetDimension == ETextureDimension::TexCube || targetDimension == ETextureDimension::TexCubeArray;
    }
    if (baseDimension == ETextureDimension::Tex3D) {
        return targetDimension == ETextureDimension::Tex3D;
    }
    return false;
}

bool RHIDynamicBuffer::operator==(const RHIDynamicBuffer& rhs) const {
    return BufferIndex == rhs.BufferIndex && Offset == rhs.Offset && Size == rhs.Size && Stride == rhs.Stride;
}

bool RHITextureSubRes::operator==(const RHITextureSubRes& rhs) const {
    return ViewFlags == rhs.ViewFlags && Dimension == rhs.Dimension &&
        MipIndex == rhs.MipIndex && MipSize == rhs.MipSize && ArrayIndex == rhs.ArrayIndex && ArraySize == rhs.ArraySize;
}

RHITextureSubRes RHITextureSubRes::Tex2D(uint16 arrayIndex, ETextureViewFlags viewFlags) {
    return { arrayIndex, 1, 0, 1, ETextureDimension::Tex2D, viewFlags };
}

uint32 RHIRenderPassInfo::GetNumColorTargets() const {
    for(uint32 i=0; i<ColorTargets.Size(); ++i) {
	    if(!ColorTargets[i].Target) {
            return i;
	    }
    }
    return RHI_COLOR_TARGET_MAX;
}

bool RHIShaderParam::operator==(const RHIShaderParam& rhs) const {
    if(Type != rhs.Type) {
        return false;
    }
    switch(Type) {
    case EBindingType::StorageBuffer:
    case EBindingType::UniformBuffer:
        if(IsDynamicBuffer != rhs.IsDynamicBuffer) {
            return false;
        }
        return IsDynamicBuffer ? (Data.DynamicBuffer == rhs.Data.DynamicBuffer) : (Data.Buffer == rhs.Data.Buffer && Data.Offset == rhs.Data.Offset && Data.Size == rhs.Data.Offset);
    case EBindingType::Texture:
        return Data.Texture == rhs.Data.Texture && Data.SubRes == rhs.Data.SubRes;
    case EBindingType::Sampler:
        return Data.Sampler == rhs.Data.Sampler;
    default: return true;
    }
}

bool RHIShaderParam::operator!=(const RHIShaderParam& rhs) const {
    return !operator==(rhs);
}

RHIShaderParam RHIShaderParam::UniformBuffer(RHIBuffer* buffer, uint32 offset, uint32 size) {
    RHIShaderParam parameter{};
    parameter.Type = EBindingType::UniformBuffer;
    parameter.Data.Buffer = buffer;
    parameter.Data.Offset = offset;
    parameter.Data.Size = size;
    return parameter;
}

RHIShaderParam RHIShaderParam::StorageBuffer(RHIBuffer* buffer, uint32 offset, uint32 size) {
    RHIShaderParam parameter{};
    parameter.Type = EBindingType::StorageBuffer;
    parameter.Data.Buffer = buffer;
    parameter.Data.Offset = offset;
    parameter.Data.Size = size;
    return parameter;
}

RHIShaderParam RHIShaderParam::UniformBuffer(const RHIDynamicBuffer& dynamicBuffer) {
    RHIShaderParam parameter{};
    parameter.Type = EBindingType::UniformBuffer;
    parameter.IsDynamicBuffer = true;
    parameter.Data.DynamicBuffer = dynamicBuffer;
    return parameter;
}

RHIShaderParam RHIShaderParam::StorageBuffer(const RHIDynamicBuffer& dynamicBuffer) {
    RHIShaderParam parameter{};
    parameter.Type = EBindingType::StorageBuffer;
    parameter.IsDynamicBuffer = true;
    parameter.Data.DynamicBuffer = dynamicBuffer;
    return parameter;
}

RHIShaderParam RHIShaderParam::Texture(RHITexture* texture, RHITextureSubRes textureSub) {
    RHIShaderParam parameter{};
    parameter.Type = EBindingType::Texture;
    parameter.Data.Texture = texture;
    parameter.Data.SubRes = textureSub;
    return parameter;
}

RHIShaderParam RHIShaderParam::Texture(RHITexture* texture) {
    return Texture(texture, texture->GetDesc().GetDefaultSubRes());
}

RHIShaderParam RHIShaderParam::Sampler(RHISampler* sampler) {
    RHIShaderParam parameter{};
    parameter.Type = EBindingType::Sampler;
    parameter.Data.Sampler = sampler;
    return parameter;
}
