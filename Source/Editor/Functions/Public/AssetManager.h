#pragma once
#include "AssetManager.h"
#include "Core/Public/String.h"
#include "Core/Public/TUniquePtr.h"
#include "Core/Public/Defines.h"
#include "Core/Public/File.h"
#include "Core/Public/Func.h"
#include "Core/Public/Container.h"
#include "Asset/Public/AssetLoader.h"
#include "Asset/Public/MeshAsset.h"

namespace Editor {

	typedef uint32 NodeID;
	typedef uint32 EventID;

	class AssetViewBase;
	class AssetManager;

	constexpr NodeID INVALID_NODE = UINT32_MAX;
	//Node Base
	class PathNode {
	protected:
		const AssetManager* m_Owner;
		NodeID m_ID{ 0 };
		NodeID m_ParentID{ INVALID_NODE };
		File::FPath m_RelativePath;//relative path
		XString m_RelativePathStr;
		XString m_Name;
		friend class AssetManager;
	public:
		PathNode(const AssetManager* owner, NodeID id) : m_Owner(owner), m_ID(id) {}
		void ResetPath(File::FPath&& relativePath, NodeID parent);
		const XString& GetName() const { return m_Name; } // name with ext
		const File::FPath& GetPath() const { return m_RelativePath; }
		File::FPath GetFullPath() const;
		const XString& GetPathStr() const { return m_RelativePathStr; }
		NodeID GetID() const { return m_ID; }
		NodeID ParentFolder() const { return m_ParentID; }
	};

	//folder
	class FolderNode : public PathNode {
	private:
		TArray<NodeID> m_Folders;
		TArray<NodeID> m_Files;
		friend class AssetManager;
	public:
		FolderNode(const AssetManager* owner, NodeID id) : PathNode(owner, id) {}
		const TArray<NodeID>& GetChildFolders() const { return m_Folders; }
		const TArray<NodeID>& GetChildFiles() const { return m_Files; }
		bool Contains(const FolderNode* folderNode) const;
		void Rename(const char* newName);
	};

	//file
	class FileNode: public PathNode {
	private:
		TArray<std::pair<EventID, Func<void(FileNode*)>>> m_OnFileChange;
		friend class AssetManager;
	public:
		FileNode(const AssetManager* owner, NodeID id): PathNode(owner, id){}
		XString GetExt() const { return m_RelativePath.extension().string(); }
		XString GetNameWithoutExt() const { return m_RelativePath.stem().string(); }
		void RenameWithoutExt(const char* newName);
	};

	class AssetManager {
	public:
		AssetManager(File::FPath&& rootPath);
		FileNode* GetFileNode(NodeID id);
		FolderNode* GetFolderNode(NodeID id);
		FileNode* GetFileNode(const File::FPath& path);
		FolderNode* GetFolderNode(const File::FPath& path);
		FolderNode* GetRootNode();
		NodeID CreateFolder(File::FPath&& relativePath, NodeID parent);
		NodeID CreateFile(File::FPath&& relativePath, NodeID parent);
		void ReloadFolder(NodeID nodeId, bool recursively);
		NodeID RootID();
		const File::FPath& GetRootPath() const { return m_RootPath; }
		void RegisterFolderModifiedEvent(void (*event)(const FolderNode*)) { m_OnFolderModified = event; }
	private:
		File::FPath m_RootPath;
		TArray<FolderNode> m_Folders;
		TArray<FileNode> m_Files;
		NodeID m_Root;
		//events
		void (*m_OnFolderModified)(const FolderNode*) { nullptr };
		void RemoveFile(NodeID id);
		void RemoveFolder(NodeID id);
		NodeID BuildFolderRecursively(const File::FPath& path, NodeID parent);
		void BuildTree();
	};

	class EngineAssetMgr: public AssetManager {
		SINGLETON_INSTANCE(EngineAssetMgr);
	private:
		EngineAssetMgr();
	};
	class ProjectAssetMgr: public AssetManager {
		SINGLETON_INSTANCE(ProjectAssetMgr);
	private:
		ProjectAssetMgr();
	};
}
