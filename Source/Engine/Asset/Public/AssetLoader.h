#pragma once
#include "AssetCommon.h"
#include "Core/Public/String.h"
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
	private:
		static XString s_EngineAssetPath;
		static XString s_ProjectAssetPath;
	public:
		static File::FPath AssetPath();
		static bool LoadProjectAsset(AssetBase* asset, File::PathStr file);
		static bool LoadEngineAsset(AssetBase* asset, File::PathStr file);
		static bool SaveProjectAsset(AssetBase* asset, File::PathStr file);
	};
}