#include "Objects/Public/RenderResource.h"
#include "Asset/Public/AssetLoader.h"
#include "Render/Public/DefaultResource.h"

namespace {
	static constexpr ETextureFlags TEXTURE_RESOURCE_FLAGS = ETextureFlags::SRV | ETextureFlags::CopyDst;

	TArray<Object::StaticResourceMgr::GraphicsPipelineInitializer>& GetGraphicsPSOInitializers() {
		static TArray<Object::StaticResourceMgr::GraphicsPipelineInitializer> s_Initializers;
		return s_Initializers;
	}
	TArray<Object::StaticResourceMgr::ComputePipelineInitializer>& GetComputePSOInitializers() {
		static TArray<Object::StaticResourceMgr::ComputePipelineInitializer> s_Initializers;
		return s_Initializers;
	}

	inline void CalcAABB(TConstArrayView<Asset::AssetVertex> vertices, Math::FVector3& outMin, Math::FVector3& outMax) {
		outMin = { FLT_MAX };
		outMax = { -FLT_MAX };
		for (auto& vertex : vertices) {
			outMin = Math::FVector3::Min(vertex.Position, outMin);
			outMax = Math::FVector3::Max(vertex.Position, outMax);
		}
	}

	inline RHITextureDesc ConvertAssetTypeToDesc(Asset::ETextureAssetType type) {
		switch (type) {
		case Asset::ETextureAssetType::R8_2D: {
			auto desc = RHITextureDesc::Texture2D();
			desc.Format = ERHIFormat::R8_UNORM;
			return desc;
		}
		case Asset::ETextureAssetType::RG8_2D: {
			auto desc = RHITextureDesc::Texture2D();
			desc.Format = ERHIFormat::R8G8_UNORM;
			return desc;
		}
		case Asset::ETextureAssetType::RGBA8Srgb_2D: {
			auto desc = RHITextureDesc::Texture2D();
			desc.Format = ERHIFormat::R8G8B8A8_SRGB;
			return desc;
		}
		case Asset::ETextureAssetType::RGBA8Srgb_Cube:
			auto desc = RHITextureDesc::TextureCube();
			desc.Format = ERHIFormat::R8G8B8A8_SRGB;
			return desc;
		}
		LOG_FATAL("Unsupported texture asset!");
	}
}

namespace Object {

	ERHIFormat TextureAssetTypeToRHIFormat(Asset::ETextureAssetType type) {
		static TStaticArray<ERHIFormat, (uint32)Asset::ETextureAssetType::MaxNum> s_FormatMap{
			ERHIFormat::R8G8B8A8_SRGB,
			ERHIFormat::R8G8_UNORM,
			ERHIFormat::R8_UNORM,
			ERHIFormat::R8G8B8A8_UNORM,
		};
		return s_FormatMap[EnumCast(type)];
	}


	RHITexturePtr StaticResourceMgr::CreateTextureFromAsset(const Asset::TextureAsset& asset) {
		RHITextureDesc desc = ConvertAssetTypeToDesc(asset.Type);
		desc.Flags = TEXTURE_RESOURCE_FLAGS;
		desc.Width = asset.Width;
		desc.Height = asset.Height;
		RHITexturePtr texture = RHI::Instance()->CreateTexture(desc);
		texture->UpdateData(asset.Pixels.ByteSize(), asset.Pixels.Data());
		return texture;
	}

	RHITexture* StaticResourceMgr::GetTexture(const XString& fileName) {
		if (!fileName.empty()) {
			if (auto iter = m_Textures.find(fileName); iter != m_Textures.end()) {
				return iter->second.Get();
			}
			Asset::TextureAsset imageAsset;
			if (Asset::AssetLoader::LoadProjectAsset(&imageAsset, fileName.c_str())) {
				RHITexturePtr texturePtr = CreateTextureFromAsset(imageAsset);
				auto* texRHI = texturePtr.Get();
				m_Textures.emplace(fileName, MoveTemp(texturePtr));
				return texRHI;
			}
			LOG_WARNING("Failed to load texture file: %s", fileName.c_str());
		}
		return Render::DefaultResources::Instance()->GetDefaultTexture2D(Render::DefaultResources::TEX_WHITE);
	}

	PrimitiveResource StaticResourceMgr::GetPrimitive(const XString& fileName) {
		if(!fileName.empty()) {
			if(auto iter = m_Primitives.find(fileName); iter != m_Primitives.end()) {
				return iter->second.GetOuter();
			}
			// create new resource
			Asset::PrimitiveAsset asset;
			if(Asset::AssetLoader::LoadProjectAsset(&asset, fileName.c_str())) {
				auto& newPrimitive = m_Primitives.emplace(fileName, PrimitiveResourceInner{}).first->second;
				const RHIBufferDesc vbDesc{ EBufferFlags::Vertex | EBufferFlags::CopyDst, asset.Vertices.ByteSize(), sizeof(Asset::AssetVertex) };
				newPrimitive.VertexBuffer = RHI::Instance()->CreateBuffer(vbDesc);
				newPrimitive.VertexBuffer->UpdateData(asset.Vertices.Data(), asset.Vertices.ByteSize(), 0);
				if(asset.Indices.Size()) {
					const RHIBufferDesc ivDesc{ EBufferFlags::Index | EBufferFlags::CopyDst, asset.Indices.ByteSize(), sizeof(Asset::IndexType) };
					newPrimitive.IndexBuffer = RHI::Instance()->CreateBuffer(ivDesc);
					newPrimitive.IndexBuffer->UpdateData(asset.Indices.Data(), asset.Indices.ByteSize(), 0);
				}
				newPrimitive.VertexCount = asset.Vertices.Size();
				newPrimitive.IndexCount = asset.Indices.Size();
				CalcAABB(asset.Vertices, newPrimitive.AABB.Min, newPrimitive.AABB.Max);
				return newPrimitive.GetOuter();
			}
			LOG_WARNING("Failed to load primitive file: %s", fileName.c_str());
		}
		return PrimitiveResource{ nullptr, nullptr, 0, 0, {} };
	}

	uint32 StaticResourceMgr::RegisterPSOInitializer(GraphicsPipelineInitializer func) {
		auto& psos = GetGraphicsPSOInitializers();
		uint32 idx = psos.Size();
		psos.PushBack(func);
		return idx;
	}

	uint32 StaticResourceMgr::RegisterPSOInitializer(ComputePipelineInitializer func) {
		auto& psos = GetComputePSOInitializers();
		uint32 idx = psos.Size();
		psos.PushBack(func);
		return idx;
	}

	RHIGraphicsPipelineState* StaticResourceMgr::GetGraphicsPipelineState(uint32 psoID) {
		return m_GraphicsPipelineStates[psoID].Get();
	}

	RHIComputePipelineState* StaticResourceMgr::GetComputePipelineState(uint32 psoID) {
		return m_ComputePipelineStates[psoID].Get();
	}

	PrimitiveResource StaticResourceMgr::PrimitiveResourceInner::GetOuter() {
		return PrimitiveResource{ VertexBuffer.Get(), IndexBuffer.Get(), VertexCount, IndexCount, AABB };
	}

	StaticResourceMgr::StaticResourceMgr() {
		RHI* r = RHI::Instance();
		auto& graphicsPSOs = GetGraphicsPSOInitializers();
		m_GraphicsPipelineStates.Reserve(graphicsPSOs.Size());
		for (auto& initializer : graphicsPSOs) {
			RHIGraphicsPipelineStateDesc desc{};
			initializer(desc);
			m_GraphicsPipelineStates.PushBack(r->CreateGraphicsPipelineState(desc));
		}
		auto& computePSOs = GetComputePSOInitializers();
		m_ComputePipelineStates.Reserve(computePSOs.Size());
		for (auto& initializer : computePSOs) {
			RHIComputePipelineStateDesc desc{};
			initializer(desc);
			m_ComputePipelineStates.PushBack(r->CreateComputePipelineState(desc));
		}
	}
}

