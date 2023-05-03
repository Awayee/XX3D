#pragma once
#include "Core/Public/Container.h"
#include "Core/Public/SmartPointer.h"
#include "Core/Public/String.h"
#include "Asset/Public/AssetCommon.h"
#include "Core/Public/BaseStructs.h"

namespace Editor {

	enum EAssetType {
		ASSET_FOLDER,
		ASSET_FILE,
		ASSET_NUM
	};

	class AssetNode {
	public:
		const static EAssetType Type = ASSET_NUM;
	protected:
		uint32 m_ID{0};
		String m_Name;
		String m_Path; // Relative path, Assets/
	public:
		AssetNode(const char* name, const char* path) : m_Name(name), m_Path(path) {}
	};

	class AssetFile: public AssetNode {
	private:
		TUniquePtr<AAssetBase> m_Asset;
	public:
		static const EAssetType Type = ASSET_FILE;
		AssetFile(const char* name, const char* path) : AssetNode(name, path) {}
	};

	class AssetFolder: public AssetNode {
	private:
		TVector<AssetNode*> m_Contents;
	public:
		static const EAssetType Type = ASSET_FOLDER;
		AssetFolder(const char* name, const char* path) : AssetNode(name, path) {}
	};
}