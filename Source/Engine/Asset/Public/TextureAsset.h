#pragma once
#include "AssetCommon.h"

namespace Asset {
	enum class ETextureAssetType{
		RGBA8_2D = 0,
		RG8_2D,
		R8_2D,
		RGBA8_Cube,
		MaxNum
	};
	enum class ETextureCompressMode : uint8 {
		None = 0,
		LZ4,
		Zlib,
	};

	struct TextureAsset : AssetBase {
		uint32 Width{ 0 };
		uint32 Height{ 0 };
		ETextureAssetType Type{ ETextureAssetType::MaxNum };
		ETextureCompressMode CompressMode{ETextureCompressMode::None};
		TArray<uint8> Pixels;
	public:
		TextureAsset() = default;
		bool Load(File::RFile& in) override;
		bool Save(File::WFile& out) override;
		~TextureAsset() override;
	};	
}
