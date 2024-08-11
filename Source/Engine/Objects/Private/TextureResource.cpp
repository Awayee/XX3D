#include "Objects/Public/TextureResource.h"

namespace {
	static constexpr ERHIFormat FORMAT = ERHIFormat::R8G8B8A8_SRGB;
	static constexpr ETextureFlags USAGE = ETextureFlagBit::TEXTURE_FLAG_SRV | ETextureFlagBit::TEXTURE_FLAG_CPY_DST;
	static constexpr int CHANNELS = 4;
	static constexpr uint32 DEFAULT_SIZE = 2;
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

	TextureResource* TextureResourceMgr::GetTexture(const XString& fileName) {
		if(auto iter = m_All.find(fileName); iter != m_All.end()) {
			return &iter->second;
		}
		Asset::TextureAsset imageAsset;

	}
}

