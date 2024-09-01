#pragma once
#include "Core/Public/Defines.h"
#include "Core/Public/COntainer.h"
#include "Core/Public/String.h"
#include "Asset/Public/TextureAsset.h"
#include "RHI/Public/RHI.h"

namespace Object {
	class TextureResource {
	public:
		NON_COPYABLE(TextureResource);
		TextureResource(const Asset::TextureAsset& asset);
		~TextureResource() = default;
		TextureResource(TextureResource&& rhs) noexcept;
		TextureResource& operator=(TextureResource&& rhs) noexcept;
		RHITexture* GetRHI();
		static ERHIFormat ConvertTextureFormat(Asset::ETextureAssetType assetType);
	private:
		RHITexturePtr m_RHI;
	};

	class TextureResourceMgr {
		SINGLETON_INSTANCE(TextureResourceMgr);
	public:
		RHITexture* GetTexture(const XString& fileName);
	private:
		TMap<XString, TextureResource> m_All;
		TextureResourceMgr() = default;
		~TextureResourceMgr() = default;
	};
}