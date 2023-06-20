#pragma once
#include "Core/Public/String.h"
#include "Core/Public/SmartPointer.h"
#include "Core/Public/Typedefine.h"
#include "Core/Public/File.h"
#include "Asset/Public/AssetLoader.h"
#include <functional>

namespace Editor {

	typedef uint32 NodeID;
	class AssetViewBase;

	constexpr NodeID INVALLID_NODE = UINT32_MAX;

	//Node Base
	class PathNode {
	protected:
		NodeID m_ID{ 0 };
		NodeID m_ParentID{ INVALLID_NODE };
		File::FPath m_Path;//relative path
		friend class AssetManager;
	public:
		PathNode(const File::FPath& path, NodeID id, NodeID parent);
		const File::FPath& GetPath() const { return m_Path; }
		NodeID GetID() const { return m_ID; }
		NodeID ParentFolder() const { return m_ParentID; }
	};

	//folder
	class FolderNode : public PathNode {
	private:
		TVector<NodeID> m_Folders;
		TVector<NodeID> m_Files;
		friend class AssetManager;
	public:
		FolderNode(const File::FPath& path, NodeID id, NodeID parent) : PathNode(path, id, parent) {}
		const TVector<NodeID>& GetChildFolders() const { return m_Folders; }
		const TVector<NodeID>& GetChildFiles() const { return m_Files; }
		bool Contains(NodeID node)const;
	};

	//file
	class FileNode: public PathNode {
	private:
		TUniquePtr<AAssetBase> m_Asset;
		friend class AssetManager;
	public:
		FileNode(const File::FPath& path, NodeID id, NodeID parent): PathNode(path, id, parent){}
		//lazy load
		template<typename T> T* GetAsset() {
			T* asset = dynamic_cast<T*>(m_Asset.get());
			if(!asset) {
				m_Asset.reset(new T);
				AssetLoader::LoadProjectAsset(m_Path.string().c_str(), m_Asset.get());
				asset = dynamic_cast<T*>(m_Asset.get());
			}
			return asset;
		}
	};

	class AssetManager {
	private:
		File::FPath m_RootPath;
		TVector<FolderNode> m_Folders;
		TVector<FileNode> m_Files;
		NodeID m_Root;
		//events
		void (*m_OnFolderRebuild)(const FolderNode*) {nullptr };
	private:
		NodeID InsertFolder(const File::FPath& path, NodeID parent);
		NodeID InsertFile(const File::FPath& path, NodeID parent);
		void RemoveFile(NodeID id);
		void RemoveFolder(NodeID id);
		NodeID BuildFolder(const File::FPath& path, NodeID parent);
	public:
		AssetManager(const char* rootPath);
		void BuildTree();
		FileNode* GetFile(NodeID id);
		FolderNode* GetFolder(NodeID id);
		FolderNode* GetRoot();

		void RegisterFolderRebuildEvent(void (*event)(const FolderNode*)) {m_OnFolderRebuild = event;}
	};

}
