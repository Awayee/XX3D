#include "EditorAsset/Public/AssetBrowser.h"
#include "Core/Public/File.h"
#include "Asset/Public/MeshAsset.h"
#include "Asset/Public/TextureAsset.h"
#include "Asset/Public/SceneAsset.h"

namespace Editor {

	File::FPath AssetBrowser::s_AssetPath = PROJECT_ASSETS;

	EFileType ConvertFileType(const char* ext) {
		if (StrEqual(ext, ".mesh")) return EFileType::MESH;
		if (StrEqual(ext, ".scene")) return EFileType::SCENE;
		if (StrEqual(ext, ".texture")) return EFileType::TEXTURE;
		return EFileType::UNKNOWN;
	}

	AssetNode::AssetNode(const char* path, AssetNode* parent): m_Path(path) {
		m_Parent = parent;
		m_Name = m_Path.stem().string();
	}

	AssetFile::AssetFile(const char* path, AssetNode* parent): AssetNode(path, parent) {
		m_FileType = ConvertFileType(m_Path.extension().string().c_str());

		switch(m_FileType) {
		case(EFileType::MESH):
			m_Asset.reset(new AMeshAsset());
			break;
		case(EFileType::TEXTURE):
			m_Asset.reset(new ATextureAsset());
			break;
		case(EFileType::SCENE):
			m_Asset.reset(new ASceneAsset());
		default:
			m_Asset = nullptr;
		}
	}

	void AssetBrowser::BuildAssetsTree() {
	}

	AssetBrowser::AssetBrowser() {
	}
}
