#pragma once
#include "AssetManager.h"
#include "Core/Public/String.h"
#include "Core/Public/TUniquePtr.h"
#include "Core/Public/Defines.h"
#include "Core/Public/File.h"
#include "Core/Public/Func.h"
#include "Asset/Public/AssetLoader.h"
#include "Asset/Public/MeshAsset.h"

namespace Editor {

	typedef uint32 NodeID;
	typedef uint32 EventID;

	class AssetViewBase;

	constexpr NodeID INVALLID_NODE = UINT32_MAX;
	//Node Base
	class PathNode {
	protected:
		NodeID m_ID{ 0 };
		NodeID m_ParentID{ INVALLID_NODE };
		File::FPath m_Path;//relative path
		XString m_PathStr;
		XString m_Name;
		XString m_Ext;
		friend class AssetManager;
	public:
		PathNode(const File::FPath& path, NodeID id, NodeID parent);
		_NODISCARD const XString& GetName() const { return m_Name; }
		_NODISCARD const File::FPath& GetPath() const { return m_Path; }
		_NODISCARD XString GetExt() const { return m_Ext; }
		_NODISCARD File::FPath GetFullPath() const;
		_NODISCARD const XString& GetPathStr() const { return m_PathStr; }
		_NODISCARD NodeID GetID() const { return m_ID; }
		_NODISCARD NodeID ParentFolder() const { return m_ParentID; }
	};

	//folder
	class FolderNode : public PathNode {
	private:
		TArray<NodeID> m_Folders;
		TArray<NodeID> m_Files;
		friend class AssetManager;
	public:
		FolderNode(const File::FPath& path, NodeID id, NodeID parent) : PathNode(path, id, parent) {}
		_NODISCARD const TArray<NodeID>& GetChildFolders() const { return m_Folders; }
		_NODISCARD const TArray<NodeID>& GetChildFiles() const { return m_Files; }
		_NODISCARD bool Contains(NodeID node)const;
	};

	//file
	class FileNode: public PathNode {
	private:
		TUniquePtr<Asset::AssetBase> m_Asset;
		TArray<std::pair<EventID, Func<void(FileNode*)>>> m_OnFileChange;
		friend class AssetManager;
	public:
		FileNode(const File::FPath& path, NodeID id, NodeID parent): PathNode(path, id, parent){}
		template<typename T> T* GetAsset() {
			if (!m_Asset.Get()) {//lazy load
				m_Asset.Reset(new T);
				Asset::AssetLoader::LoadProjectAsset(m_Asset.Get(), m_Path.string().c_str());
			}
			return static_cast<T*>(m_Asset.Get());
		}
		void Save();
	};

	class AssetManager {
	private:
		File::FPath m_RootPath;
		TArray<FolderNode> m_Folders;
		TArray<FileNode> m_Files;
		NodeID m_Root;
		//events
		void (*m_OnFolderRebuild)(const FolderNode*) {nullptr };
	private:
		NodeID InsertFolder(const File::FPath& path, NodeID parent);
		NodeID InsertFile(const File::FPath& path, NodeID parent);
		void RemoveFile(NodeID id);
		void RemoveFolder(NodeID id);
		NodeID BuildFolderRecursively(const File::FPath& path, NodeID parent);
	public:
		AssetManager(const char* rootPath);
		void BuildTree();
		FileNode* GetFile(NodeID id);
		FileNode* GetFile(const File::FPath& path);
		FolderNode* GetFolder(NodeID id);
		FolderNode* GetRoot();
		NodeID RootID();
		void RegisterFolderRebuildEvent(void (*event)(const FolderNode*)) {m_OnFolderRebuild = event;}
		void ImportAsset(const char* srcFile, const char* dstFile);//srcFile is absolute path, dstFile is relative path
	};


	class EngineAssetMgr: public AssetManager {
		SINGLETON_INSTANCE(EngineAssetMgr);
	private:
		EngineAssetMgr(): AssetManager(ENGINE_ASSETS){}
	};
	class ProjectAssetMgr: public AssetManager {
		SINGLETON_INSTANCE(ProjectAssetMgr);
	private:
		ProjectAssetMgr(): AssetManager(PROJECT_ASSETS){}
	};

}
