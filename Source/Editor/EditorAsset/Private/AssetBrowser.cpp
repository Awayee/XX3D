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

	PathNode::PathNode(const File::FPath& path, NodeID id, NodeID parent): m_Path(path), m_ID(id), m_ParentID(parent) {
		m_Name = m_Path.stem().string();
	}

	bool FolderNode::Contains(const FolderNode* node) const {
		return m_ID < node->m_ID;
	}

	FileNode::FileNode(const File::FPath& path, NodeID id, NodeID parent): PathNode(path, id, parent) {
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

	NodeID AssetBrowser::InsertFolder(const File::FPath& path, NodeID parent) {
		const NodeID id = UINT32_CAST(m_Folders.size());
		m_Folders.emplace_back(path, id, parent);
		FolderNode* parentNode = GetFolder(parent);
		if(parentNode){
			parentNode->m_Folders.push_back(id);
		}
		return id;
	}

	NodeID AssetBrowser::InsertFile(const File::FPath& path, NodeID parent) {
		const NodeID id = UINT32_CAST(m_Files.size());
		m_Files.emplace_back(path, id, parent);
		FolderNode* parentNode = GetFolder(parent);
		if(parentNode){
			parentNode->m_Files.push_back(id);
		}
		return id;
	}


	void AssetBrowser::RemoveFile(NodeID id) {
		FileNode* node = GetFile(id);
		if(!node) {
			return;
		}
		FolderNode* parentFolder = GetFolder(node->m_ParentID);
		if (parentFolder) {
			SwapRemove(parentFolder->m_Files, id);
		}
		PathNode* swappedAsset = &m_Files.back();
		SwapRemoveAt(m_Files, id);

		if (swappedAsset->m_ParentID != INVALLID_NODE) {
			FolderNode* backParent = GetFolder(swappedAsset->m_ParentID);
			Replace(backParent->m_Files, swappedAsset->m_ID, id);
			swappedAsset->m_ID = id;
		}
	}

	void AssetBrowser::RemoveFolder(NodeID id) {
		FolderNode* node = GetFolder(id);
		if(!node) {
			return;
		}
		FolderNode* parentFolder = GetFolder(node->m_ParentID);
		if(parentFolder) {
			SwapRemove(parentFolder->m_Folders, id);
		}
		PathNode* swappedAsset = &m_Folders.back();
		SwapRemoveAt(m_Folders, id);

		if(swappedAsset && swappedAsset->m_ParentID != INVALLID_NODE) {
			FolderNode* backParent = GetFolder(swappedAsset->m_ParentID);
			Replace(backParent->m_Folders, swappedAsset->m_ID, id);
			swappedAsset->m_ID = id;
		}
	}

	NodeID AssetBrowser::BuildFolder(const File::FPath& path, NodeID parent) {
		using namespace File;
		//the folder node
		NodeID folder = InsertFolder(path, parent);
		FPathIterator iter(path);
		for(const FPathEntry& child: iter) {
			if(child.is_directory()) {
				BuildFolder(child, folder);
			}
			else {
				InsertFile(child.path(), folder);
			}
		}
		if(m_OnFolderRebuild) {
			m_OnFolderRebuild(GetFolder(folder));
		}
		return folder;
	}

	void AssetBrowser::BuildTree() {
		m_Folders.clear();
		m_Files.clear();
		m_Root = BuildFolder(s_AssetPath, INVALLID_NODE);
	}

	FileNode* AssetBrowser::GetFile(NodeID id) {
		return id < m_Files.size() ? &m_Files[id] : nullptr;
	}

	FolderNode* AssetBrowser::GetFolder(NodeID id) {
		return id < m_Folders.size() ? &m_Folders[id] : nullptr;
	}

	FolderNode* AssetBrowser::GetRoot() {
		return GetFolder(m_Root);
	}
}
