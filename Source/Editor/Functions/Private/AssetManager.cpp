#include "Functions/Public/AssetManager.h"
#include "Core/Public/File.h"
#include "Core/Public/Log.h"
#include "Core/Public/String.h"
#include "Functions/Public/TextureImporter.h"
#include "Functions/Public/MeshImporter.h"

namespace Editor {
	
	PathNode::PathNode(const File::FPath& path, NodeID id, NodeID parent):m_ID(id), m_ParentID(parent), m_Path(path) {
		m_PathStr = m_Path.string();
		std::replace(m_PathStr.begin(), m_PathStr.end(), '\\', '/');
		m_Name = m_Path.filename().string();
	}

	File::FPath PathNode::GetFullPath() const {
		File::FPath path{ Asset::AssetLoader::AssetPath() };
		path.append(m_Path.string());
		return path;
	}


	bool FolderNode::Contains(NodeID id) const {
		return m_ID < id;
	}

	void FileNode::Save() {
		if(Asset::AssetLoader::SaveProjectAsset(m_Asset.Get(), m_PathStr.c_str())) {
			LOG_INFO("FileNode::Save %s", GetPathStr().c_str());
		}
	}

	NodeID AssetManager::InsertFolder(const File::FPath& path, NodeID parent) {
		const NodeID id = static_cast<NodeID>(m_Folders.Size());
		m_Folders.PushBack({ File::RelativePath(path, m_RootPath), id, parent });
		FolderNode* parentNode = GetFolder(parent);
		if(parentNode){
			parentNode->m_Folders.PushBack(id);
		}
		return id;
	}

	NodeID AssetManager::InsertFile(const File::FPath& path, NodeID parent) {
		const NodeID id = static_cast<NodeID>(m_Files.Size());
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

	NodeID AssetManager::BuildFolderRecursively(const File::FPath& path, NodeID parent) {
		using namespace File;
		//the folder node
		NodeID folder = InsertFolder(path, parent);
		FPathIterator iter(path);
		for(const FPathEntry& child: iter) {
			if(child.is_directory()) {
				BuildFolderRecursively(child, folder);
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
		m_Folders.Reset();
		m_Files.Reset();
		m_Root = BuildFolderRecursively(m_RootPath, INVALLID_NODE);
		m_Folders[m_Root].m_Name = m_RootPath.parent_path().stem().string();
	}

	FileNode* AssetManager::GetFile(NodeID id) {
		return id < m_Files.Size() ? &m_Files[id] : nullptr;
	}

	FileNode* AssetManager::GetFile(const File::FPath& path) {
		FolderNode* folder = GetRoot();
		for(auto iter: path) {
			if(!folder) {
				break;
			}
			//foreach all files
			if(iter.has_extension()) {
				for (NodeID childID : folder->GetChildFiles()) {
					FileNode* childFile = GetFile(childID);
					if (childFile->GetPath().filename() == iter) {
						return childFile;
					}
				}
			}
			else {
				for (NodeID childID : folder->GetChildFolders()) {
					FolderNode* childFolder = GetFolder(childID);
					auto stem = childFolder->GetPath().stem();
					if (stem == iter) {
						folder = childFolder;
					}
				}
			}
		}
		LOG_INFO("AssetManager::GetFile failed: %s", path.string().c_str());
		return nullptr;
	}

	FolderNode* AssetManager::GetFolder(NodeID id) {
		return id < m_Folders.Size() ? &m_Folders[id] : nullptr;
	}

	FolderNode* AssetManager::GetRoot() {
		return GetFolder(m_Root);
	}

	NodeID AssetManager::RootID() {
		return m_Root;
	}

	void AssetManager::ImportAsset(const char* srcFile, const char* dstFile) {
		if(StrEndsWith(srcFile, ".png")) {
			Asset::TextureAsset asset;
			TextureImporter importer(&asset, dstFile);
			importer.Import(srcFile);
			importer.Save();
		}
		else if (StrEndsWith(srcFile, ".glb") || StrEndsWith(srcFile, "fbx")) {
			Asset::MeshAsset asset;
			MeshImporter importer(&asset, dstFile);
			importer.Import(srcFile);
			importer.Save();
		}
	}
}
