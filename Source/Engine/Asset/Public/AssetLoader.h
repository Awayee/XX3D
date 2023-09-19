#pragma once
#include "AssetCommon.h"
#include "Core/Public/String.h"
#include "Core/Public/TVector.h"
#include "Core/Public/File.h"

namespace Engine {
	enum EAssetType : uint8 {
		ASSET_TYPE_MESH = 0,
		ASSET_TYPE_IMAGE,
		ASSET_TYPE_SCENE
	};


	class AssetLoader {
		// all path are relative
	private:
		static XXString s_EngineAssetPath;
		static XXString s_ProjectAssetPath;
	public:
		static File::FPath AssetPath();
		static bool LoadProjectAsset(AAssetBase* asset, File::PathStr file);
		static bool LoadEngineAsset(AAssetBase* asset, File::PathStr file);
		static bool SaveProjectAsset(AAssetBase* asset, File::PathStr file);
	};
}