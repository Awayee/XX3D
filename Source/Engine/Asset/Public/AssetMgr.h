#pragma once
#include "Core/Public/String.h"
#include "Core/Public/Container.h"
#include "Core/Public/File.h"

#define PARSE_PROJECT_ASSET(f)\
	char __s[128]; strcpy(__s, PROJECT_ASSETS); strcat(__s, f); f=__s

#define PARSE_ENGINE_ASSET(f)\
	char __s[128]; strcpy(__s, ENGINE_ASSETS); strcat(__s, f); f=__s

enum EAssetType : uint8 {
	ASSET_TYPE_MESH = 0,
	ASSET_TYPE_IMAGE,
	ASSET_TYPE_SCENE
};


class AssetMgr {
	// all path are relative
private:
	static String s_EngineAssetPath;
	static String s_ProjectAssetPath;
public:
	template<typename T> static T LoadAsset(const String& file);
	template<typename T> static T LoadEngineAsset(const String& file);
	static void ConvertProjectPath(String& file);
	static void GetFiles(const String& folder, TVector<String>& files);
};

struct Asset {
	EAssetType Type;
	int Version;
	String JsonFile;
	String FilePath;
	TVector<char> Binary;
	virtual bool Load(const char* file);
	virtual bool Save(const char* file);
};
