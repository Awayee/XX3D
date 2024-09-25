#include "D3D12Util.h"
#include "Core/Public/Log.h"
#include "Core/Public/String.h"

void WINAPI DXTraceW(_In_z_ const WCHAR* strFile, _In_ DWORD dwLine, _In_ HRESULT hr, _In_opt_ const WCHAR* strMsg) {
    XString strMsg0 = WString2String(strMsg);
    WCHAR e[256];
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, hr, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), e, 256, nullptr);
	LOG_ERROR("[DXTraceW]%ls: %lu 0x%0.8x %s %ls", strFile, dwLine, hr, strMsg0.c_str(), e);
}

inline uint32 Align(uint32 size, uint32 alignment) {
    return (size + (alignment - 1)) & ~(alignment - 1);
}

DXGI_FORMAT ToD3D12Format(ERHIFormat f) {
    static DXGI_FORMAT s_D3D11FormatMap[(uint32)ERHIFormat::FORMAT_MAX_ENUM]{
        DXGI_FORMAT_UNKNOWN,
        DXGI_FORMAT_R8_UNORM,
        DXGI_FORMAT_R8_SNORM,
        DXGI_FORMAT_R8_UINT,
        DXGI_FORMAT_R8_SINT,
        DXGI_FORMAT_R8G8_UNORM,
        DXGI_FORMAT_R8G8_SNORM,
        DXGI_FORMAT_R8G8_UINT,
        DXGI_FORMAT_R8G8_SINT,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_R8G8B8A8_SNORM,
        DXGI_FORMAT_R8G8B8A8_UINT,
        DXGI_FORMAT_R8G8B8A8_SINT,
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        DXGI_FORMAT_R16_UNORM,
        DXGI_FORMAT_R16_SNORM,
        DXGI_FORMAT_R16_UINT,
        DXGI_FORMAT_R16_SINT,
        DXGI_FORMAT_R16_FLOAT,
        DXGI_FORMAT_R16G16_UNORM,
        DXGI_FORMAT_R16G16_SNORM,
        DXGI_FORMAT_R16G16_UINT,
        DXGI_FORMAT_R16G16_SINT,
        DXGI_FORMAT_R16G16_FLOAT,
        DXGI_FORMAT_R16G16B16A16_UNORM,
        DXGI_FORMAT_R16G16B16A16_SNORM,
        DXGI_FORMAT_R16G16B16A16_UINT,
        DXGI_FORMAT_R16G16B16A16_SINT,
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        DXGI_FORMAT_R32_UINT,
        DXGI_FORMAT_R32_SINT,
        DXGI_FORMAT_R32_FLOAT,
        DXGI_FORMAT_R32G32_UINT,
        DXGI_FORMAT_R32G32_SINT,
        DXGI_FORMAT_R32G32_FLOAT,
        DXGI_FORMAT_R32G32B32_UINT,
        DXGI_FORMAT_R32G32B32_SINT,
        DXGI_FORMAT_R32G32B32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_UINT,
        DXGI_FORMAT_R32G32B32A32_SINT,
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_D16_UNORM,
        DXGI_FORMAT_D24_UNORM_S8_UINT,
        DXGI_FORMAT_UNKNOWN, //FORMAT_D32_SFLOAT not supported
    };
    return s_D3D11FormatMap[(uint32)f];
}

D3D12_RESOURCE_DIMENSION ToD3D12Dimension(ETextureDimension dimension) {
    switch (dimension) {
    case ETextureDimension::Tex2D:
    case ETextureDimension::Tex2DArray:
    case ETextureDimension::TexCube:
    case ETextureDimension::TexCubeArray: return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    case ETextureDimension::Tex3D: return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
    default: return D3D12_RESOURCE_DIMENSION_UNKNOWN;
    }
}

D3D12_HEAP_TYPE ToD3D12HeapTypeBuffer(EBufferFlags flags) {
    if (EnumHasAnyFlags(flags, EBufferFlags::CopySrc | EBufferFlags::Uniform)) {
        return D3D12_HEAP_TYPE_UPLOAD;
    }
    if (EnumHasAnyFlags(flags, EBufferFlags::Readback)) {
        return D3D12_HEAP_TYPE_READBACK;
    }
    return D3D12_HEAP_TYPE_DEFAULT;
}


D3D12_HEAP_TYPE ToD3D12HeapTypeTexture(ETextureFlags flags) {
    if (EnumHasAnyFlags(flags, ETextureFlags::CopySrc)) {
        return D3D12_HEAP_TYPE_UPLOAD;
    }
    return D3D12_HEAP_TYPE_DEFAULT;
}

D3D12_RESOURCE_FLAGS ToD3D12ResourceFlags(ETextureFlags flags) {
    D3D12_RESOURCE_FLAGS d3d12Flags = D3D12_RESOURCE_FLAG_NONE;
    if(EnumHasAnyFlags(flags, ETextureFlags::ColorTarget)) {
        d3d12Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    if(EnumHasAnyFlags(flags, ETextureFlags::DepthStencilTarget)) {
        d3d12Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    }
    if(EnumHasAnyFlags(flags, ETextureFlags::UAV)) {
        d3d12Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    return d3d12Flags; 
}

D3D12_RESOURCE_STATES ToD3D12InitialStateBuffer(EBufferFlags flags) {
    if (EnumHasAnyFlags(flags, EBufferFlags::CopySrc)) {
        return D3D12_RESOURCE_STATE_COPY_SOURCE;
    }
    if (EnumHasAnyFlags(flags, EBufferFlags::CopyDst)) {
        return D3D12_RESOURCE_STATE_COPY_DEST;
    }
    return D3D12_RESOURCE_STATE_COMMON;
}

D3D12_RESOURCE_STATES ToD3D12InitialStateTexture(ETextureFlags flags) {
    if (EnumHasAnyFlags(flags, ETextureFlags::CopyDst)) {
        return D3D12_RESOURCE_STATE_COPY_DEST;
    }
    if (EnumHasAnyFlags(flags, ETextureFlags::Present)) {
        return D3D12_RESOURCE_STATE_PRESENT;
    }
    if (EnumHasAnyFlags(flags, ETextureFlags::ColorTarget)) {
        return D3D12_RESOURCE_STATE_COMMON;
    }
    if (EnumHasAnyFlags(flags, ETextureFlags::DepthStencilTarget)) {
        return D3D12_RESOURCE_STATE_COMMON;
    }
    return D3D12_RESOURCE_STATE_COMMON;
}

D3D12_FILTER ToD3D12Filter(ESamplerFilter filter) {
    switch (filter) {
    case ESamplerFilter::Point: return D3D12_FILTER_MIN_MAG_MIP_POINT;
    case ESamplerFilter::Bilinear: return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    case ESamplerFilter::Trilinear: return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    case ESamplerFilter::AnisotropicPoint:
    case ESamplerFilter::AnisotropicLinear: return D3D12_FILTER_ANISOTROPIC;
    default: return D3D12_FILTER_MIN_MAG_MIP_POINT;
    }
}

D3D12_TEXTURE_ADDRESS_MODE ToD3D12TextureAddressMode(ESamplerAddressMode addressMode) {
    switch (addressMode) {
    case ESamplerAddressMode::Wrap: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    case ESamplerAddressMode::Clamp: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    case ESamplerAddressMode::Mirror: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
    case ESamplerAddressMode::Border: return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    default: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    }
}

D3D12_COMMAND_LIST_TYPE ToD3D12CommandListType(EQueueType type) {
    switch (type) {
    case EQueueType::Graphics: return D3D12_COMMAND_LIST_TYPE_DIRECT;
    case EQueueType::Compute: return D3D12_COMMAND_LIST_TYPE_COMPUTE;
    case EQueueType::Transfer: return D3D12_COMMAND_LIST_TYPE_COPY;
    default: return D3D12_COMMAND_LIST_TYPE_DIRECT;
    }
}

D3D12_LOGIC_OP ToD3D12LogicOp(ELogicOp op) {
    switch (op) {
    case ELogicOp::Clear: return D3D12_LOGIC_OP_CLEAR;
    case ELogicOp::And: return D3D12_LOGIC_OP_AND;
    case ELogicOp::AndReverse: return D3D12_LOGIC_OP_AND_REVERSE;
    case ELogicOp::Copy: return D3D12_LOGIC_OP_COPY;
    case ELogicOp::AndInverted: return D3D12_LOGIC_OP_AND_INVERTED;
    case ELogicOp::NoOp: return D3D12_LOGIC_OP_NOOP;
    case ELogicOp::XOr: return D3D12_LOGIC_OP_XOR;
    case ELogicOp::Or: return D3D12_LOGIC_OP_OR;
    case ELogicOp::NOr: return D3D12_LOGIC_OP_NOR;
    case ELogicOp::Equivalent: return D3D12_LOGIC_OP_EQUIV;
    case ELogicOp::Invert: return D3D12_LOGIC_OP_INVERT;
    case ELogicOp::OrReverse: return D3D12_LOGIC_OP_OR_REVERSE;
    case ELogicOp::CopyInverted: return D3D12_LOGIC_OP_COPY_INVERTED;
    case ELogicOp::OrInverted: return D3D12_LOGIC_OP_OR_INVERTED;
    case ELogicOp::NAnd: return D3D12_LOGIC_OP_NAND;
    case ELogicOp::Set: return D3D12_LOGIC_OP_SET;
    }
    return D3D12_LOGIC_OP_NOOP;
}

D3D12_BLEND_OP ToD3D12BlendOp(EBlendOption op) {
    switch (op) {
    case EBlendOption::Add: return D3D12_BLEND_OP_ADD;
    case EBlendOption::Sub: return D3D12_BLEND_OP_SUBTRACT;
    case EBlendOption::Min: return D3D12_BLEND_OP_MIN;
    case EBlendOption::Max: return D3D12_BLEND_OP_MAX;
    case EBlendOption::ReverseSub: return D3D12_BLEND_OP_REV_SUBTRACT;
    }
    return D3D12_BLEND_OP_ADD;
}

D3D12_BLEND ToD3D12Blend(EBlendFactor factor) {
    switch (factor) {
    case EBlendFactor::Zero: return D3D12_BLEND_ZERO;
    case EBlendFactor::One: return D3D12_BLEND_ONE;
    case EBlendFactor::SrcColor: return D3D12_BLEND_SRC_COLOR;
    case EBlendFactor::InverseSrcColor: return D3D12_BLEND_INV_SRC_COLOR;
    case EBlendFactor::DstColor: return D3D12_BLEND_DEST_COLOR;
    case EBlendFactor::InverseDstColor: return D3D12_BLEND_INV_DEST_COLOR;
    case EBlendFactor::SrcAlpha: return D3D12_BLEND_SRC_ALPHA;
    case EBlendFactor::InverseSrcAlpha: return D3D12_BLEND_INV_SRC_ALPHA;
    case EBlendFactor::DstAlpha: return D3D12_BLEND_DEST_ALPHA;
    case EBlendFactor::InverseDstAlpha: return D3D12_BLEND_INV_DEST_ALPHA;
    case EBlendFactor::ConstColor: return D3D12_BLEND_BLEND_FACTOR;
    case EBlendFactor::InverseConstColor: return D3D12_BLEND_INV_BLEND_FACTOR;
    case EBlendFactor::ConstAlpha: return D3D12_BLEND_BLEND_FACTOR;
    case EBlendFactor::InverseConstAlpha: return D3D12_BLEND_INV_BLEND_FACTOR;
    }
    return D3D12_BLEND_ONE;
}

UINT8 ToD3D12ComponentFlags(EColorComponentFlags flags) {
    UINT8 dstFlags = 0;
    if (EnumHasAnyFlags(flags, EColorComponentFlags::R)) {
        dstFlags |= D3D12_COLOR_WRITE_ENABLE_RED;
    }
    if (EnumHasAnyFlags(flags, EColorComponentFlags::G)) {
        dstFlags |= D3D12_COLOR_WRITE_ENABLE_GREEN;
    }
    if (EnumHasAnyFlags(flags, EColorComponentFlags::B)) {
        dstFlags |= D3D12_COLOR_WRITE_ENABLE_BLUE;
    }
    if (EnumHasAnyFlags(flags, EColorComponentFlags::A)) {
        dstFlags |= D3D12_COLOR_WRITE_ENABLE_ALPHA;
    }
    return dstFlags;
}

D3D12_FILL_MODE ToD3D12FillMode(ERasterizerFill fill) {
    switch (fill) {
    case ERasterizerFill::Solid: return D3D12_FILL_MODE_SOLID;
    case ERasterizerFill::Wireframe: return D3D12_FILL_MODE_WIREFRAME;
    default: return D3D12_FILL_MODE_SOLID;
    }
}

D3D12_CULL_MODE ToD3D12CullMode(ERasterizerCull cull) {
    switch (cull) {
    case ERasterizerCull::Back: return D3D12_CULL_MODE_BACK;
    case ERasterizerCull::Front: return D3D12_CULL_MODE_FRONT;
    case ERasterizerCull::Null: return D3D12_CULL_MODE_NONE;
    }
    return D3D12_CULL_MODE_NONE;
}

D3D12_COMPARISON_FUNC ToD3D12Comparison(ECompareType type) {
    switch (type) {
    case ECompareType::Less: return D3D12_COMPARISON_FUNC_LESS;
    case ECompareType::LessEqual: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
    case ECompareType::Greater: return D3D12_COMPARISON_FUNC_GREATER;
    case ECompareType::GreaterEqual: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    case ECompareType::Equal: return D3D12_COMPARISON_FUNC_EQUAL;
    case ECompareType::NotEqual: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
    case ECompareType::Never: return D3D12_COMPARISON_FUNC_NEVER;
    case ECompareType::Always: return D3D12_COMPARISON_FUNC_ALWAYS;
    }
    return D3D12_COMPARISON_FUNC_ALWAYS;
}

D3D12_STENCIL_OP ToD3D12StencilOp(EStencilOp op) {
    switch (op) {
    case EStencilOp::Keep: return D3D12_STENCIL_OP_KEEP;
    case EStencilOp::Zero: return D3D12_STENCIL_OP_ZERO;
    case EStencilOp::Replace: return D3D12_STENCIL_OP_REPLACE;
    case EStencilOp::SaturatedIncrement: return D3D12_STENCIL_OP_INCR_SAT;
    case EStencilOp::SaturatedDecrement: return D3D12_STENCIL_OP_DECR_SAT;
    case EStencilOp::Invert: return D3D12_STENCIL_OP_INVERT;
    case EStencilOp::Increment: return D3D12_STENCIL_OP_INCR;
    case EStencilOp::Decrement: return D3D12_STENCIL_OP_DECR;
    }
    return D3D12_STENCIL_OP_KEEP;
}

D3D12_DEPTH_STENCILOP_DESC ToD3D12DepthStencilOpDesc(const RHIDepthStencilState::StencilOpDesc& desc) {
    D3D12_DEPTH_STENCILOP_DESC d3d12Desc;
    d3d12Desc.StencilDepthFailOp = ToD3D12StencilOp(desc.DepthFailOp);
    d3d12Desc.StencilFailOp = ToD3D12StencilOp(desc.FailOp);
    d3d12Desc.StencilPassOp = ToD3D12StencilOp(desc.PassOp);
    d3d12Desc.StencilFunc = ToD3D12Comparison(desc.CompareType);
    return d3d12Desc;
}

D3D12_PRIMITIVE_TOPOLOGY_TYPE ToD3D12PrimitiveTopologyType(EPrimitiveTopology topology) {
    switch (topology) {
    case EPrimitiveTopology::TriangleList:
    case EPrimitiveTopology::TriangleStrip: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    case EPrimitiveTopology::PointList: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    case EPrimitiveTopology::LineList:
    case EPrimitiveTopology::LineStrip: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    }
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
}

D3D12_PRIMITIVE_TOPOLOGY ToD3D12PrimitiveTopology(EPrimitiveTopology topology) {
    switch(topology) {
    case EPrimitiveTopology::TriangleList: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case EPrimitiveTopology::TriangleStrip: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    case EPrimitiveTopology::PointList: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    case EPrimitiveTopology::LineList: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    case EPrimitiveTopology::LineStrip: return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
    }
    return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

D3D12_DESCRIPTOR_RANGE_TYPE ToD3D12DescriptorRangeType(EBindingType type) {
    switch (type) {
    case EBindingType::UniformBuffer: return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    case EBindingType::Texture: return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    case EBindingType::StorageTexture: return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    case EBindingType::Sampler: return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
    }
    return D3D12_DESCRIPTOR_RANGE_TYPE_CBV; // error
}

uint32 GetD3D12PerArraySliceSize(ETextureDimension dimension) {
    switch (dimension) {
    case ETextureDimension::Tex2D:
    case ETextureDimension::Tex3D:
    case ETextureDimension::Tex2DArray: return 1;
    case ETextureDimension::TexCube:
    case ETextureDimension::TexCubeArray: return 6;
    default:
        LOG_ERROR("Invalid view dimension!");
        return 1;
    }
}

D3D12_RESOURCE_STATES ToD3D12ResourceState(EResourceState state) {
    switch (state) {
    case EResourceState::ColorTarget: return D3D12_RESOURCE_STATE_RENDER_TARGET;
    case EResourceState::DepthStencil: return D3D12_RESOURCE_STATE_DEPTH_WRITE;
    case EResourceState::ShaderResourceView: return D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
    case EResourceState::UnorderedAccessView:  return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    case EResourceState::TransferSrc:  return D3D12_RESOURCE_STATE_COPY_SOURCE;
    case EResourceState::TransferDst: return D3D12_RESOURCE_STATE_COPY_DEST;
    case EResourceState::Present: return D3D12_RESOURCE_STATE_PRESENT;
    }
    return D3D12_RESOURCE_STATE_COMMON;
}

D3D12_DESCRIPTOR_HEAP_TYPE ToD3D12DescriptorHeapType(EBindingType type) {
    switch (type) {
    case EBindingType::UniformBuffer:
    case EBindingType::StorageBuffer:
    case EBindingType::Texture:
    case EBindingType::StorageTexture: return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    case EBindingType::Sampler: return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    }
    return D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
}

uint32 AlignConstantBufferSize(uint32 byteSize) {
    // Constant buffers must be a multiple of the minimum hardware allocation size (usually 256 bytes).  So round up to nearest multiple of 256.
    return Align(byteSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
}

uint32 AlignTexturePitchSize(uint32 byteSize) {
    return Align(byteSize, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

}

uint32 AlignTextureSliceSize(uint32 byteSize) {
    return Align(byteSize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
}

DXGI_FORMAT ToD3D12ViewFormat(ERHIFormat textureFormat, ETextureViewFlags viewFlags) {
    if(viewFlags == ETextureViewFlags::Depth) {
	    if(textureFormat == ERHIFormat::D24_UNORM_S8_UINT) {
            return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	    }
    }
    else if(viewFlags == ETextureViewFlags::Stencil) {
	    if(textureFormat == ERHIFormat::D24_UNORM_S8_UINT) {
            return DXGI_FORMAT_X24_TYPELESS_G8_UINT;
	    }
    }
    else if(viewFlags == ETextureViewFlags::DepthStencil) {
        if (textureFormat == ERHIFormat::D24_UNORM_S8_UINT) {
            return ToD3D12Format(textureFormat);
        }
    }
    else if (viewFlags == ETextureViewFlags::Color) {
        return ToD3D12Format(textureFormat);
    }
    return DXGI_FORMAT_UNKNOWN;
}

D3D12_RESOURCE_STATES ToD3D12DestResourceState(EBufferFlags flags) {
    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
    if (EnumHasAnyFlags(flags, EBufferFlags::Index)) {
        state |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
    }
    if (EnumHasAnyFlags(flags, EBufferFlags::Vertex)) {
        state |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    }
    return state;
}

D3D12_RESOURCE_STATES ToD3D12DestResourceState(ETextureFlags flags) {
    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
    if(EnumHasAnyFlags(flags, ETextureFlags::SRV)) {
        state |= D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
    }
    if(EnumHasAnyFlags(flags, ETextureFlags::UAV)) {
        state |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }
    if(EnumHasAnyFlags(flags, ETextureFlags::ColorTarget)) {
        state |= D3D12_RESOURCE_STATE_RENDER_TARGET;
    }
    if(EnumHasAnyFlags(flags, ETextureFlags::DepthStencilTarget)) {
        state |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
    }
    return state;
}
