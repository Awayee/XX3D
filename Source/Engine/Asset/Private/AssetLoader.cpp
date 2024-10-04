#include "Asset/Public/AssetLoader.h"
#include "Core/Public/Log.h"

namespace Asset {

	XString AssetLoader::s_EngineAssetPath{ ENGINE_ASSETS };
	XString AssetLoader::s_ProjectAssetPath{ PROJECT_ASSETS };

	namespace {
		// TODO Temporary to check the file content, all assets will be binary
		bool BinaryFile(File::PathStr file) {
			return StrEndsWith(file, ".level") || StrEndsWith(file, ".model");
		}
	}

	File::FPath AssetLoader::AssetPath() {
		return s_ProjectAssetPath;
	}

	bool AssetLoader::LoadProjectAsset(AssetBase* asset, File::PathStr filePath) {
		File::FPath fullPath(s_ProjectAssetPath);
		fullPath.append(filePath);
		if(!asset->Load(fullPath.string().c_str())) {
			LOG_WARNING("[AssetLoader::LoadProjectAsset] Failed to load file: %s", filePath);
			return false;
		}
		return true;
	}

	bool AssetLoader::LoadEngineAsset(AssetBase* asset, File::PathStr filePath) {
		File::FPath fullPath(s_EngineAssetPath);
		fullPath.append(filePath);
		if (!asset->Load(fullPath.string().c_str())) {
			LOG_WARNING("[AssetLoader::LoadEngineAsset] Failed to load file: %s", filePath);
			return false;
		}
		return true;
	}

	bool AssetLoader::SaveProjectAsset(AssetBase* asset, File::PathStr filePath) {
		File::FPath fullPath(s_ProjectAssetPath);
		fullPath.append(filePath);
		if (!asset->Save(fullPath.string().c_str())) {
			LOG_WARNING("[AssetLoader::SaveProjectAsset] Failed to save file: %s", filePath);
			return false;
		}
		return true;
	}

}
