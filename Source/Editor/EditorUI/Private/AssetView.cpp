#include "AssetView.h"
#include "Functions/Public/EditorLevelMgr.h"

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

	void TextureAssetView::Open() {
	}

	void LevelAssetView::Open() {
		EditorLevelMgr::Instance()->LoadLevel(m_Asset, m_Node->GetPath());

	}

	void LevelAssetView::Save() {
	}
}
