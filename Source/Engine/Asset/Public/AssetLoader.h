#pragma once
#include "AssetCommon.h"
#include "Core/Public/TArray.h"
#include "Core/Public/File.h"

namespace Asset {
	enum EAssetType : uint8 {
		ASSET_TYPE_MESH = 0,
		ASSET_TYPE_IMAGE,
		ASSET_TYPE_SCENE
	};

	class AssetLoader {
		// all path are relative
	public:
		static File::FPath AssetPath();
		static File::FPath GetRelativePath(File::PathStr fullPath);
		// the "filePath" above means relative path
		static File::FPath GetAbsolutePath(File::PathStr filePath);
		static bool LoadProjectAsset(AssetBase* asset, File::PathStr filePath);
		static bool LoadEngineAsset(AssetBase* asset, File::PathStr filePath);
		static bool SaveProjectAsset(AssetBase* asset, File::PathStr filePath);
	};
}