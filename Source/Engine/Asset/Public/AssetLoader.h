#pragma once
#include "AssetCommon.h"
#include "Core/Public/String.h"
#include "Core/Public/TVector.h"
#include "Core/Public/File.h"

enum EAssetType : uint8 {
	ASSET_TYPE_MESH = 0,
	ASSET_TYPE_IMAGE,
	ASSET_TYPE_SCENE
};


class AssetLoader {
	// all path are relative
private:
	static String s_EngineAssetPath;
	static String s_ProjectAssetPath;
public:
	static File::FPath AssetPath();
	static bool LoadProjectAsset(File::CharPath file, AAssetBase* asset);
	static bool LoadEngineAsset(File::CharPath file, AAssetBase* asset);
	static bool SaveProjectAsset(File::CharPath file, AAssetBase* asset);
};
