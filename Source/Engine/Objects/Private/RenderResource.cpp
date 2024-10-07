#include "Objects/Public/RenderResource.h"
#include "Asset/Public/AssetLoader.h"
#include "Render/Public/DefaultResource.h"

namespace {
	static constexpr ERHIFormat FORMAT = ERHIFormat::R8G8B8A8_SRGB;
	static constexpr ETextureFlags USAGE = ETextureFlags::SRV | ETextureFlags::CopyDst;
	static constexpr int CHANNELS = 4;
}

namespace Object {
	TextureResource::TextureResource(const Asset::TextureAsset& asset) {
		RHITextureDesc desc = RHITextureDesc::Texture2D();
		desc.Format = ConvertTextureFormat(asset.Type);
		desc.Flags = USAGE;
		desc.Width = asset.Width;
		desc.Height = asset.Height;
		m_RHI = RHI::Instance()->CreateTexture(desc);
		m_RHI->UpdateData(desc.Width * desc.Height * CHANNELS, asset.Pixels.Data());
	}

	TextureResource::TextureResource(TextureResource&& rhs) noexcept : m_RHI(MoveTemp(rhs.m_RHI)) {}

	TextureResource& TextureResource::operator=(TextureResource&& rhs) noexcept {
		m_RHI = MoveTemp(rhs.m_RHI);
		return *this;
	}

	RHITexture* TextureResource::GetRHI() {
		return m_RHI;
	}

	ERHIFormat TextureResource::ConvertTextureFormat(Asset::ETextureAssetType assetType) {
		static TStaticArray<ERHIFormat, (uint32)Asset::ETextureAssetType::MaxNum> s_FormatMap{
			ERHIFormat::R8G8B8A8_SRGB,
			ERHIFormat::R8G8_UNORM,
			ERHIFormat::R8_UNORM,
			ERHIFormat::R8G8B8A8_UNORM,
		};
		return s_FormatMap[(uint32)assetType];
	}

	RHITexture* TextureResourceMgr::GetTexture(const XString& fileName) {
		if(!fileName.empty()) {
			if (auto iter = m_All.find(fileName); iter != m_All.end()) {
				return iter->second.GetRHI();
			}
			Asset::TextureAsset imageAsset;
			if (Asset::AssetLoader::LoadProjectAsset(&imageAsset, fileName.c_str())) {
				TextureResource texRes{ imageAsset };
				auto* texRHI = texRes.GetRHI();
				m_All.emplace(fileName, MoveTemp(texRes));
				return texRHI;
			}
			LOG_WARNING("Failed to load texture file: %s", fileName.c_str());
		}
		return Render::DefaultResources::Instance()->GetDefaultTexture2D(Render::DefaultResources::TEX_WHITE);
	}

	TArray<StaticPipelineStateMgr::GraphicsPipelineInitializer>& GetGraphicsPSOInitializers() {
		static TArray<StaticPipelineStateMgr::GraphicsPipelineInitializer> s_Initializers;
		return s_Initializers;
	}
	TArray<StaticPipelineStateMgr::ComputePipelineInitializer>&  GetComputePSOInitializers() {
		static TArray<StaticPipelineStateMgr::ComputePipelineInitializer> s_Initializers;
		return s_Initializers;
	}

	uint32 StaticPipelineStateMgr::RegisterPSOInitializer(GraphicsPipelineInitializer func) {
		auto& psos = GetGraphicsPSOInitializers();
		uint32 idx = psos.Size();
		psos.PushBack(func);
		return idx;
	}

	uint32 StaticPipelineStateMgr::RegisterPSOInitializer(ComputePipelineInitializer func) {
		auto& psos = GetComputePSOInitializers();
		uint32 idx = psos.Size();
		psos.PushBack(func);
		return idx;
	}

	void StaticPipelineStateMgr::ReCreatePipelineState(uint32 psoID, const RHIGraphicsPipelineStateDesc& desc) {
		CHECK(psoID < m_GraphicsPipelineStates.Size());
		m_GraphicsPipelineStates[psoID] = RHI::Instance()->CreateGraphicsPipelineState(desc);
	}

	RHIGraphicsPipelineState* StaticPipelineStateMgr::GetGraphicsPipelineState(uint32 psoID) {
		return m_GraphicsPipelineStates[psoID].Get();
	}

	RHIComputePipelineState* StaticPipelineStateMgr::GetComputePipelineState(uint32 psoID) {
		return m_ComputePipelineStates[psoID].Get();
	}

	StaticPipelineStateMgr::StaticPipelineStateMgr() {
		RHI* r = RHI::Instance();
		auto& graphicsPSOs = GetGraphicsPSOInitializers();
		m_GraphicsPipelineStates.Reserve(graphicsPSOs.Size());
		for(auto& initializer: graphicsPSOs) {
			RHIGraphicsPipelineStateDesc desc{};
			initializer(desc);
			m_GraphicsPipelineStates.PushBack(r->CreateGraphicsPipelineState(desc));
		}
		auto& computePSOs = GetComputePSOInitializers();
		m_ComputePipelineStates.Reserve(computePSOs.Size());
		for(auto& initializer: computePSOs) {
			RHIComputePipelineStateDesc desc{};
			initializer(desc);
			m_ComputePipelineStates.PushBack(r->CreateComputePipelineState(desc));
		}
	}
}

