#pragma once
#include "Core/Public/String.h"
#include "Core/Public/SmartPointer.h"
#include "Core/Public/Typedefine.h"
#include "Asset/Public/AssetCommon.h"
#include "Core/Public/File.h"

namespace Editor {

	class AssetNode {
	protected:
		uint32 m_ID{ 0 };
		String m_Name;
		File::FPath m_Path;
		AssetNode* m_Parent;
	public:
		AssetNode(const char* path, AssetNode* parent);
	};

	class AssetFolder : public AssetNode {
	private:
		TVector<AssetNode*> m_Children;
	public:
		AssetFolder(const char* path, AssetNode* parent) : AssetNode(path, parent) {}
	};

	enum class EFileType : uint8 {
		MESH,
		SCENE,
		TEXTURE,
		UNKNOWN
	};
	class AssetFile: public AssetNode {
	private:
		TUniquePtr<AAssetBase> m_Asset;
		EFileType m_FileType;
	public:
		AssetFile(const char* path, AssetNode* parent);
	};

	class AssetBrowser {
	private:
		static File::FPath s_AssetPath;
		void BuildAssetsTree();
	public:
		AssetBrowser();
	};
}