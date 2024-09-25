#pragma once
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dx12.h>
#include <wrl.h>
#include "Core/Public/Log.h"
#include "Core/Public/TUniquePtr.h"
#include "RHI/Public/RHIEnum.h"
#include "RHI/Public/RHIResources.h"
#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

/*DXTraceW(__FILEW__, (DWORD)__LINE__, hr, L#x);\*/
void WINAPI DXTraceW(_In_z_ const WCHAR* strFile, _In_ DWORD dwLine, _In_ HRESULT hr, _In_opt_ const WCHAR* strMsg);

#if defined(DEBUG) | defined(_DEBUG)
#define DX_CHECK(x)\
    do{\
        HRESULT hr = (x);\
        if(FAILED(hr)){\
			DXTraceW(__FILEW__, (DWORD)__LINE__, hr, L#x);\
			ASSERT(0, "");\
        }\
    }while(0)
#else
#define DX_CHECK(x) (x)
#endif

template<typename T>
struct TDXDeleter {  void operator()(T* ptr) { ptr->Release(); } };

template<typename T>
using TDXPtr = TUniquePtr<T, TDXDeleter<T>>;

static constexpr uint32 DX_INVALID_INDEX = UINT32_MAX;

// converter
DXGI_FORMAT ToD3D12Format(ERHIFormat f);
D3D12_RESOURCE_DIMENSION ToD3D12Dimension(ETextureDimension dimension);
D3D12_HEAP_TYPE ToD3D12HeapTypeBuffer(EBufferFlags flags);
D3D12_HEAP_TYPE ToD3D12HeapTypeTexture(ETextureFlags flags);
D3D12_RESOURCE_FLAGS ToD3D12ResourceFlags(ETextureFlags flags);
D3D12_RESOURCE_STATES ToD3D12InitialStateBuffer(EBufferFlags flags);
D3D12_RESOURCE_STATES ToD3D12InitialStateTexture(ETextureFlags flags);
D3D12_FILTER ToD3D12Filter(ESamplerFilter filter);
D3D12_TEXTURE_ADDRESS_MODE ToD3D12TextureAddressMode(ESamplerAddressMode addressMode);
D3D12_COMMAND_LIST_TYPE ToD3D12CommandListType(EQueueType type);
D3D12_LOGIC_OP ToD3D12LogicOp(ELogicOp op);
D3D12_BLEND_OP ToD3D12BlendOp(EBlendOption op);
D3D12_BLEND ToD3D12Blend(EBlendFactor factor);
UINT8 ToD3D12ComponentFlags(EColorComponentFlags flags);
D3D12_FILL_MODE ToD3D12FillMode(ERasterizerFill fill);
D3D12_CULL_MODE ToD3D12CullMode(ERasterizerCull cull);
D3D12_COMPARISON_FUNC ToD3D12Comparison(ECompareType type);
D3D12_STENCIL_OP ToD3D12StencilOp(EStencilOp op);
D3D12_DEPTH_STENCILOP_DESC ToD3D12DepthStencilOpDesc(const RHIDepthStencilState::StencilOpDesc& desc);
D3D12_PRIMITIVE_TOPOLOGY_TYPE ToD3D12PrimitiveTopologyType(EPrimitiveTopology topology);
D3D12_PRIMITIVE_TOPOLOGY ToD3D12PrimitiveTopology(EPrimitiveTopology topology);
D3D12_DESCRIPTOR_RANGE_TYPE ToD3D12DescriptorRangeType(EBindingType type);
uint32 GetD3D12PerArraySliceSize(ETextureDimension dimension);
D3D12_RESOURCE_STATES ToD3D12ResourceState(EResourceState state);
D3D12_DESCRIPTOR_HEAP_TYPE ToD3D12DescriptorHeapType(EBindingType type);
uint32 AlignConstantBufferSize(uint32 byteSize);
uint32 AlignTexturePitchSize(uint32 byteSize);
uint32 AlignTextureSliceSize(uint32 byteSize);
DXGI_FORMAT ToD3D12ViewFormat(ERHIFormat textureFormat, ETextureViewFlags viewFlags);
D3D12_RESOURCE_STATES ToD3D12DestResourceState(EBufferFlags flags);
D3D12_RESOURCE_STATES ToD3D12DestResourceState(ETextureFlags flags);