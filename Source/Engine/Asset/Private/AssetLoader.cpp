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

	bool AssetLoader::LoadProjectAsset(AssetBase* asset, File::PathStr file) {
		File::FPath filePath(s_ProjectAssetPath);
		filePath.append(file);
		File::RFile in(filePath.string().c_str(), BinaryFile(file) ? (File::EFileMode::Read) : (File::EFileMode::Read | File::EFileMode::Binary));
		if (!in.is_open()) {
			LOG_INFO("[AssetLoader::LoadProjectAsset] Failed to read file: %s", filePath.string().c_str());
			return false;
		}

		bool ok = asset->Load(in);
		if (!ok) {
			LOG_INFO("[AssetLoader::LoadProjectAsset] Failed to load asset: %s", file);
		}
		return ok;
	}

	bool AssetLoader::LoadEngineAsset(AssetBase* asset, File::PathStr file) {
		File::FPath filePath(s_EngineAssetPath);
		filePath.append(file);
		File::RFile in(filePath.string().c_str(), BinaryFile(file) ? (File::EFileMode::Read) : (File::EFileMode::Read | File::EFileMode::Binary));
		if (!in.is_open()) {
			LOG_INFO("[AssetLoader::LoadEngineAsset] Failed to read file: %s", filePath.string().c_str());
			return false;
		}

		bool ok = asset->Load(in);
		if (!ok) {
			LOG_INFO("[AssetLoader::LoadEngineAsset] Failed to load asset: %s", file);
		}
		return ok;
	}

	bool AssetLoader::SaveProjectAsset(AssetBase* asset, File::PathStr file) {
		File::FPath filePath(s_ProjectAssetPath);
		filePath.append(file);
		File::WFile out(filePath.string().c_str(), BinaryFile(file) ? (File::EFileMode::Write) : (File::EFileMode::Write | File::EFileMode::Binary));
		if (!out.is_open()) {
			LOG_INFO("[AssetLoader::SaveProjectAsset] Failed to write file: %s", filePath.string().c_str());
			return false;
		}

		bool ok = asset->Save(out);
		if (!ok) {
			LOG_INFO("[AssetLoader::SaveProjectAsset] Failed to save asseet: %s", file);
		}
		return ok;
	}

}
