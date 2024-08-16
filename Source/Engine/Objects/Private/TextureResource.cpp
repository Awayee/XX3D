#include "Objects/Public/TextureResource.h"
#include "Asset/Public/AssetLoader.h"
#include "Render/Public/DefaultResource.h"

namespace {
	static constexpr ERHIFormat FORMAT = ERHIFormat::R8G8B8A8_SRGB;
	static constexpr ETextureFlags USAGE = ETextureFlagBit::TEXTURE_FLAG_SRV | ETextureFlagBit::TEXTURE_FLAG_CPY_DST;
	static constexpr int CHANNELS = 4;
}

namespace Object {
	TextureResource::TextureResource(const Asset::TextureAsset& asset) {
		RHITextureDesc desc = RHITextureDesc::Texture2D();
		desc.Format = FORMAT;
		desc.Flags = USAGE;
		desc.Width = asset.Width;
		desc.Height = asset.Height;
		m_RHI = RHI::Instance()->CreateTexture(desc);
		m_RHI->UpdateData(desc.Width * desc.Height * CHANNELS, asset.Pixels.Data(), {});
	}

	TextureResource::TextureResource(TextureResource&& rhs) noexcept : m_RHI(MoveTemp(rhs.m_RHI)) {}

	TextureResource& TextureResource::operator=(TextureResource&& rhs) noexcept {
		m_RHI = MoveTemp(rhs.m_RHI);
		return *this;
	}

	RHITexture* TextureResource::GetRHI() {
		return m_RHI;
	}

	RHITexture* TextureResourceMgr::GetTexture(const XString& fileName) {
		if(auto iter = m_All.find(fileName); iter != m_All.end()) {
			return iter->second.GetRHI();
		}
		Asset::TextureAsset imageAsset;
		if(Asset::AssetLoader::LoadProjectAsset(&imageAsset, fileName.c_str())) {
			TextureResource texRes{ imageAsset };
			auto* texRHI = texRes.GetRHI();
			m_All.emplace(fileName, MoveTemp(texRes));
			return texRHI;
		}
		LOG_WARNING("Failed to load texture file: %s", fileName.c_str());
		return Render::DefaultResources::Instance()->GetDefaultTexture2D(Render::DefaultResources::TEX_WHITE);
	}
}

