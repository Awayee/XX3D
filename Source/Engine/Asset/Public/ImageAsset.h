#pragma once
#include "AssetCommon.h"

struct AImageAsset : AAssetBase {
	int Width;
	int Height;
	int Channels;
	uint8* Pixels;
	void Load(const char* file) override;
	void Save(const char* file) override;
	~AImageAsset() override;
};
