#pragma once
#include "AssetCommon.h"

namespace Asset {
	enum class ETextureAssetType{
		Unknown,
		RGBA8_2D,
		RGB8_2D,
		RG8_2D,
		R8_2D,
		RGBA8_Cube,
	};

	struct TextureAsset : AssetBase {
		uint32 Width{ 0 };
		uint32 Height{ 0 };
		ETextureAssetType Type{ETextureAssetType::Unknown };
		TArray<uint8> Pixels;
	public:
		TextureAsset() = default;
		bool Load(File::RFile& in) override;
		bool Save(File::WFile& out) override;
		~TextureAsset() override;
	};	
}
