#include "EditorAsset/Public/AssetManager.h"
#include "Core/Public/File.h"

namespace Editor {
	
	PathNode::PathNode(const File::FPath& path, NodeID id, NodeID parent):m_ID(id), m_ParentID(parent), m_Path(path) {
	}

	bool FolderNode::Contains(NodeID id) const {
		return m_ID < id;
	}

	NodeID AssetManager::InsertFolder(const File::FPath& path, NodeID parent) {
		const NodeID id = UINT32_CAST(m_Folders.Size());
		m_Folders.PushBack({ File::RelativePath(path, m_RootPath), id, parent });
		FolderNode* parentNode = GetFolder(parent);
		if(parentNode){
			parentNode->m_Folders.PushBack(id);
		}
		return id;
	}

	NodeID AssetManager::InsertFile(const File::FPath& path, NodeID parent) {
		const NodeID id = UINT32_CAST(m_Files.Size());
		m_Files.PushBack({ File::RelativePath(path, m_RootPath), id, parent});
		FolderNode* parentNode = GetFolder(parent);
		if(parentNode){
			parentNode->m_Files.PushBack(id);
		}
		return id;
	}


	void AssetManager::RemoveFile(NodeID id) {
		FileNode* node = GetFile(id);
		if(!node) {
			return;
		}
		FolderNode* parentFolder = GetFolder(node->m_ParentID);
		parentFolder->m_Files.SwapRemove(id);
		parentFolder->m_Files.SwapRemove(id);
		PathNode* swappedAsset = &m_Files.Back();
		m_Files.SwapRemoveAt(id);

		if (swappedAsset->m_ParentID != INVALLID_NODE) {
			FolderNode* backParent = GetFolder(swappedAsset->m_ParentID);
			backParent->m_Files.Replace(swappedAsset->m_ID, id);
			swappedAsset->m_ID = id;
		}
	}

	void AssetManager::RemoveFolder(NodeID id) {
		FolderNode* node = GetFolder(id);
		if(!node) {
			return;
		}
		FolderNode* parentFolder = GetFolder(node->m_ParentID);
		parentFolder->m_Folders.SwapRemove(id);
		PathNode* swappedAsset = &m_Folders.Back();
		m_Folders.SwapRemoveAt(id);

		if(swappedAsset && swappedAsset->m_ParentID != INVALLID_NODE) {
			FolderNode* backParent = GetFolder(swappedAsset->m_ParentID);
			backParent->m_Folders.Replace(swappedAsset->m_ID, id);
			swappedAsset->m_ID = id;
		}
	}

	NodeID AssetManager::BuildFolder(const File::FPath& path, NodeID parent) {
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

	AssetManager::AssetManager(const char* rootPath) {
		m_RootPath = rootPath;
		BuildTree();
	}

	void AssetManager::BuildTree() {
		m_Folders.Clear();
		m_Files.Clear();
		m_Root = BuildFolder(m_RootPath, INVALLID_NODE);
	}

	FileNode* AssetManager::GetFile(NodeID id) {
		return id < m_Files.Size() ? &m_Files[id] : nullptr;
	}

	FolderNode* AssetManager::GetFolder(NodeID id) {
		return id < m_Folders.Size() ? &m_Folders[id] : nullptr;
	}

	FolderNode* AssetManager::GetRoot() {
		return GetFolder(m_Root);
	}
}
