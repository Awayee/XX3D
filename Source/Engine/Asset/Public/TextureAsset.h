#pragma once
#include "AssetCommon.h"

struct ATextureAsset : AAssetBase {
	uint32 Width{0};
	uint32 Height{0};
	uint8 Channels{0};
	TVector<uint8> Pixels;
public:
	ATextureAsset() = default;
	bool Load(const char* file) override;
	bool Save(const char* file) override;
	~ATextureAsset() override;
};
