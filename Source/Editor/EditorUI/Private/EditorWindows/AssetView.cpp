#include "AssetView.h"

namespace Editor {
	FolderAssetView::FolderAssetView(FolderNode* node): m_Node(node) {
		m_Name = m_Node->GetPath().filename().string();
		m_ID = m_Node->GetID();
	}

	bool FolderAssetView::IsFolder() {
		return true;
	}

	void FolderAssetView::Open() {
	}

	void FolderAssetView::Close() {
	}

	void FolderAssetView::Save() {
	}

	FileAssetView::FileAssetView(FileNode* node): m_Node(node) {
		m_Name = m_Node->GetPath().filename().string();
		m_Icon = m_Node->GetPath().extension().string();
		m_ID = m_Node->GetID();
	}

	bool FileAssetView::IsFolder() {
		return false;
	}

	void MeshAssetView::Open() {
		LOG("You opened a mesh asset.");
	}

	void MeshAssetView::Close() {
		LOG("You closed a mesh asset.");
	}
}
