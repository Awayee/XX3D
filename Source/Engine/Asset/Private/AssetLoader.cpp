#include "Asset/Public/AssetLoader.h"
#include "Core/Public/macro.h"

namespace Engine {

	String AssetLoader::s_EngineAssetPath{ ENGINE_ASSETS };
	String AssetLoader::s_ProjectAssetPath{ PROJECT_ASSETS };

	namespace {
		// TODO Temporary to check the file content, all assets will be binary
		bool BinaryFile(File::PathStr file) {
			return StrEndsWith(file, ".level") || StrEndsWith(file, ".model");
		}
	}

	File::FPath AssetLoader::AssetPath() {
		return s_ProjectAssetPath;
	}

	bool AssetLoader::LoadProjectAsset(AAssetBase* asset, File::PathStr file) {
		File::FPath filePath(s_ProjectAssetPath);
		filePath.append(file);
		File::Read in(filePath.string().c_str(), BinaryFile(file) ? (std::ios::in) : (std::ios::in | std::ios::binary));
		if (!in.is_open()) {
			LOG("[AssetLoader::LoadProjectAsset] Failed to read file: %s", filePath.string().c_str());
			return false;
		}

		bool ok = asset->Load(in);
		if (!ok) {
			LOG("[AssetLoader::LoadProjectAsset] Failed to load asset: %s", file);
		}
		return ok;
	}

	bool AssetLoader::LoadEngineAsset(AAssetBase* asset, File::PathStr file) {
		File::FPath filePath(s_EngineAssetPath);
		filePath.append(file);
		File::Read in(filePath.string().c_str(), BinaryFile(file) ? (std::ios::in) : (std::ios::in | std::ios::binary));
		if (!in.is_open()) {
			LOG("[AssetLoader::LoadEngineAsset] Failed to read file: %s", filePath.string().c_str());
			return false;
		}

		bool ok = asset->Load(in);
		if (!ok) {
			LOG("[AssetLoader::LoadEngineAsset] Failed to load asset: %s", file);
		}
		return ok;
	}

	bool AssetLoader::SaveProjectAsset(AAssetBase* asset, File::PathStr file) {
		File::FPath filePath(s_ProjectAssetPath);
		filePath.append(file);
		File::Write out(filePath.string().c_str(), BinaryFile(file) ? (std::ios::out) : (std::ios::out | std::ios::binary));
		if (!out.is_open()) {
			LOG("[AssetLoader::SaveProjectAsset] Failed to write file: %s", filePath.string().c_str());
			return false;
		}

		bool ok = asset->Save(out);
		if (!ok) {
			LOG("[AssetLoader::SaveProjectAsset] Failed to save asseet: %s", file);
		}
		return ok;
	}

}
