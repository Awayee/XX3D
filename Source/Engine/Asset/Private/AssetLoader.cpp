#include "Asset/Public/AssetLoader.h"
#include "System/Public/Configuration.h"
#include "Core/Public/Log.h"

namespace Asset {

	namespace {
		// TODO Temporary to check the file content, all assets will be binary
		bool BinaryFile(File::PathStr file) {
			return StrEndsWith(file, ".level") || StrEndsWith(file, ".model");
		}
	}

	File::FPath AssetLoader::AssetPath() {
		return Engine::EngineConfig::Instance().GetProjectAssetDir();
	}

	File::FPath AssetLoader::GetRelativePath(File::PathStr fullPath) {
		return File::RelativePath(fullPath, Engine::EngineConfig::Instance().GetProjectAssetDir());
	}

	File::FPath AssetLoader::GetAbsolutePath(File::PathStr filePath) {
		return Engine::EngineConfig::Instance().GetProjectAssetDir().append(filePath);
	}

	bool AssetLoader::LoadProjectAsset(AssetBase* asset, File::PathStr filePath) {
		const XString fullPath = AssetPath().append(filePath).string();
		if(!asset->Load(fullPath.c_str())) {
			LOG_WARNING("[AssetLoader::LoadProjectAsset] Failed to load file: %s", filePath);
			return false;
		}
		return true;
	}

	bool AssetLoader::LoadEngineAsset(AssetBase* asset, File::PathStr filePath) {
		const XString fullPath = Engine::EngineConfig::Instance().GetEngineAssetDir().append(filePath).string();
		if (!asset->Load(fullPath.c_str())) {
			LOG_WARNING("[AssetLoader::LoadEngineAsset] Failed to load file: %s", filePath);
			return false;
		}
		return true;
	}

	bool AssetLoader::SaveProjectAsset(AssetBase* asset, File::PathStr filePath) {
		const XString fullPath = AssetPath().append(filePath).string();
		if (!asset->Save(fullPath.c_str())) {
			LOG_WARNING("[AssetLoader::SaveProjectAsset] Failed to save file: %s", filePath);
			return false;
		}
		return true;
	}

}
