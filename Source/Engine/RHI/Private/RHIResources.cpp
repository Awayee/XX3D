#include "RHI/Public/RHIResources.h"

struct FormatTexelInfo {
    ERHIFormat Format;
    uint32 PixelByteSize;
};
FormatTexelInfo g_FormatPixelByteSize[(uint32)ERHIFormat::FORMAT_MAX_ENUM] = {
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
    {ERHIFormat::B8G8R8A8_UNORM,       4             },
    {ERHIFormat::B8G8R8A8_SRGB,        4             },
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
    {ERHIFormat::R32G32B32_UINT,       12             },
    {ERHIFormat::R32G32B32_SINT,       12             },
    {ERHIFormat::R32G32B32_SFLOAT,     12             },
    {ERHIFormat::R32G32B32A32_UINT,    16             },
    {ERHIFormat::R32G32B32A32_SINT,    16             },
    {ERHIFormat::R32G32B32A32_SFLOAT,  16             },
    {ERHIFormat::D16_UNORM,            2             },
    {ERHIFormat::D24_UNORM_S8_UINT,    4             },
    {ERHIFormat::D32_SFLOAT,           4             },
};


uint32 RHITexture::GetPixelByteSize() {
    return g_FormatPixelByteSize[(uint32)m_Desc.Format].PixelByteSize;
}