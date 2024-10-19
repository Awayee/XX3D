#pragma once
#include "Core/Public/Defines.h"
#include "Core/Public/COntainer.h"
#include "Core/Public/String.h"
#include "Asset/Public/TextureAsset.h"
#include "Asset/Public/MeshAsset.h"
#include "RHI/Public/RHI.h"

namespace Object {

	ERHIFormat TextureAssetTypeToRHIFormat(Asset::ETextureAssetType type);

	struct PrimitiveResource {
		RHIBuffer* VertexBuffer;
		RHIBuffer* IndexBuffer;
		uint32 VertexCount;
		uint32 IndexCount;
		Math::AABB3 AABB;
	};

	class StaticResourceMgr {
		SINGLETON_INSTANCE(StaticResourceMgr);
	public:

		// texture
		static RHITexturePtr CreateTextureFromAsset(const Asset::TextureAsset& asset);
		RHITexture* GetTexture(const XString& fileName);

		// primitive
		PrimitiveResource GetPrimitive(const XString& fileName);

		// pipeline state
		typedef void(*ComputePipelineInitializer)(RHIComputePipelineStateDesc&);
		typedef void(*GraphicsPipelineInitializer)(RHIGraphicsPipelineStateDesc& desc);
		static uint32 RegisterPSOInitializer(GraphicsPipelineInitializer func);
		static uint32 RegisterPSOInitializer(ComputePipelineInitializer func);
		RHIGraphicsPipelineState* GetGraphicsPipelineState(uint32 psoID);
		RHIComputePipelineState* GetComputePipelineState(uint32 psoID);
	private:
		struct PrimitiveResourceInner {
			RHIBufferPtr VertexBuffer;
			RHIBufferPtr IndexBuffer;
			uint32 VertexCount;
			uint32 IndexCount;
			Math::AABB3 AABB;
			PrimitiveResource GetOuter();
		};
		TMap<XString, PrimitiveResourceInner> m_Primitives;
		TMap<XString, RHITexturePtr> m_Textures;
		TArray<RHIGraphicsPipelineStatePtr> m_GraphicsPipelineStates;
		TArray<RHIComputePipelineStatePtr> m_ComputePipelineStates;
		StaticResourceMgr();
	};
}