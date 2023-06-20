#include "Asset/Public/AssetLoader.h"
#include "Core/Public/macro.h"

String AssetLoader::s_EngineAssetPath{ ENGINE_ASSETS };
String AssetLoader::s_ProjectAssetPath{ PROJECT_ASSETS };

File::FPath AssetLoader::AssetPath() {
	return s_ProjectAssetPath;
}

bool AssetLoader::LoadProjectAsset(File::CharPath file, AAssetBase* asset) {
	File::FPath filePath(s_ProjectAssetPath);
	filePath.append(file);
	File::Read in(filePath.string().c_str(), std::ios::binary);
	if(!in.is_open()) {
		LOG("[AssetLoader::LoadProjectAsset] Failed to read file: %s", filePath.string().c_str());
		return false;
	}

	bool ok = asset->Load(in);
	if(!ok) {
		LOG("[AssetLoader::LoadProjectAsset] Failed to load asset: %s", file);
	}
	return ok;
}

bool AssetLoader::LoadEngineAsset(File::CharPath file, AAssetBase* asset) {
	File::FPath filePath(s_EngineAssetPath);
	filePath.append(file);
	File::Read in(filePath.string().c_str(), std::ios::binary);
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

bool AssetLoader::SaveProjectAsset(File::CharPath file, AAssetBase* asset) {
	File::FPath filePath(s_ProjectAssetPath);
	filePath.append(file);
	File::Write out(filePath.string().c_str(), std::ios::binary);
	if(!out.is_open()) {
		LOG("[AssetLoader::SaveProjectAsset] Failed to write file: %s", filePath.string().c_str());
		return false;
	}

	bool ok = asset->Save(out);
	if(!ok) {
		LOG("[AssetLoader::SaveProjectAsset] Failed to save asseet: %s", file);
	}
	return ok;
}
