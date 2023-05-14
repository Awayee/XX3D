#pragma once
#include "Core/Public/String.h"
#include "Core/Public/SmartPointer.h"
#include "Core/Public/Typedefine.h"
#include "Asset/Public/AssetCommon.h"
#include "Core/Public/File.h"
#include <functional>

namespace Editor {

	typedef uint32 NodeID;

	constexpr NodeID INVALLID_NODE = UINT32_MAX;

	//template<typename T>struct NodeID { uint32 ID; };

	//typedef NodeID<class FolderNode> NodeID;
	//typedef NodeID<class FileNode> NodeID;

	class PathNode {
	protected:
		NodeID m_ID{ 0 };
		NodeID m_ParentID{ INVALLID_NODE };
		String m_Name;
		File::FPath m_Path;
		friend class AssetBrowser;
	public:
		PathNode(const File::FPath& path, NodeID id, NodeID parent);
		const File::FPath& GetPath() const { return m_Path; }
		NodeID GetID() const { return m_ID; }
		NodeID ParentFolder() const { return m_ParentID; }
		const String& GetName()const { return m_Name; }
	};

	class FolderNode : public PathNode {
	private:
		//TVector<NodeID> m_Children;
		TVector<NodeID> m_Folders;
		TVector<NodeID> m_Files;
		friend class AssetBrowser;
	public:
		FolderNode(const File::FPath& path, NodeID id, NodeID parent) : PathNode(path, id, parent) {}
		const TVector<NodeID>& GetChildFolders() const { return m_Folders; }
		const TVector<NodeID>& GetChildFiles() const { return m_Files; }
		bool Contains(const FolderNode* node)const;
	};

	enum class EFileType : uint8 {
		MESH,
		SCENE,
		TEXTURE,
		UNKNOWN
	};
	class FileNode: public PathNode {
	private:
		TUniquePtr<AAssetBase> m_Asset;
		EFileType m_FileType;
		friend class AssetBrowser;
	public:
		FileNode(const File::FPath& path, NodeID id, NodeID parent);
		AAssetBase* GetAsset() const { return m_Asset.get(); }
		EFileType GetType() const { return m_FileType; }
	};

	class AssetBrowser {
	private:
		static File::FPath s_AssetPath;

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
		AssetBrowser()=default;
		void BuildTree();
		FileNode* GetFile(NodeID id);
		FolderNode* GetFolder(NodeID id);
		FolderNode* GetRoot();

		void RegisterFolderRebuildEvent(void (*event)(const FolderNode*)) {m_OnFolderRebuild = event;}
	};

}
