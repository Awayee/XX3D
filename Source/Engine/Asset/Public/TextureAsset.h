#pragma once
#include "AssetCommon.h"

namespace Engine {

	struct ATextureAsset : AAssetBase {
		uint32 Width{ 0 };
		uint32 Height{ 0 };
		uint8 Channels{ 0 };
		TVector<uint8> Pixels;
	public:
		ATextureAsset() = default;
		bool Load(File::Read& in) override;
		bool Save(File::Write& out) override;
		~ATextureAsset() override;
	};
	
}
