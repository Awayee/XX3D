#pragma once
#include "AssetCommon.h"

namespace Asset {

	struct TextureAsset : AssetBase {
		uint32 Width{ 0 };
		uint32 Height{ 0 };
		uint8 Channels{ 0 };
		TArray<uint8> Pixels;
	public:
		TextureAsset() = default;
		bool Load(File::RFile& in) override;
		bool Save(File::WFile& out) override;
		~TextureAsset() override;
	};
	
}
