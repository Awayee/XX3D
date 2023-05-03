#include "Asset/Public/AssetMgr.h"
#include "Asset/Public/AssetCommon.h"
#include "Asset/Public/ImageAsset.h"
#include "Asset/Public/MeshAsset.h"
#include "Asset/Public/SceneAsset.h"
#include "Core/Public/File.h"

#define REGISTER_ASSET_TYPE(cls) template cls AssetMgr::LoadAsset(const String& file); template cls AssetMgr::LoadEngineAsset(const String& file)

template <typename T> T AssetMgr::LoadAsset(const String& file) {
	const char* filePtr = file.c_str();
	PARSE_PROJECT_ASSET(filePtr);
	T asset;
	asset.Load(filePtr);
	return asset; // will be destructed in debug
}


template <typename T> T AssetMgr::LoadEngineAsset(const String& file) {
	const char* filePtr = file.c_str();
	PARSE_ENGINE_ASSET(filePtr);
	T asset;
	asset.Load(filePtr);
	return asset;
}


REGISTER_ASSET_TYPE(AImageAsset);

void AssetMgr::ConvertProjectPath(String& file) {
	String assetPath(PROJECT_ASSETS);
	assetPath.append(file);
	file.swap(assetPath);
}

bool Asset::Load(const char* file) {
	PARSE_PROJECT_ASSET(file);
	std::ifstream iFile;
	iFile.open(file, std::ios::binary);

	if(!iFile.is_open()) {
		return false;
	}
	iFile.seekg(0);
	iFile.read((char*)&Type, 1);
	iFile.read((char*)&Version, sizeof(uint32));

	uint32 jsonlen = 0;
	iFile.read((char*)&jsonlen, sizeof(uint32));

	uint32 bloblen = 0;
	iFile.read((char*)&bloblen, sizeof(uint32));

	JsonFile.resize(jsonlen);
	iFile.read(JsonFile.data(), jsonlen);

	Binary.resize(bloblen);
	iFile.read(Binary.data(), bloblen);

	iFile.close();
	return true;
}

bool Asset::Save(const char* file) {
	PARSE_PROJECT_ASSET(file);
	std::ofstream oFile;
	oFile.open(file, std::ios::binary | std::ios::out);
	if (!oFile.is_open())
	{
		std::cout << "Error when trying to write file: " << file << std::endl;
	}
	oFile.write((const char*)Type, 1);
	uint32 version = Version;
	//version
	oFile.write((const char*)&version, sizeof(uint32));

	//json length
	uint32 length = static_cast<uint32>(JsonFile.size());
	oFile.write((const char*)&length, sizeof(uint32));

	//blob length
	uint32 bloblenght = static_cast<uint32>(Binary.size());
	oFile.write((const char*)&bloblenght, sizeof(uint32));

	//json stream
	oFile.write(JsonFile.data(), length);
	//pixel data
	oFile.write(Binary.data(), bloblenght);

	oFile.close();

	return true;
}
