#include "AssetView.h"
#include "Functions/Public/EditorLevel.h"
#include "EditorUI/Public/EditorUIMgr.h"
#include "EditorUI/Public/EditorWindow.h"
#include "UIExtent.h"

namespace Editor {
	FolderAssetView::FolderAssetView(FolderNode* node): m_Node(node) {
		m_ID = m_Node->GetID();
	}

	const XString& FolderAssetView::GetName() {
		return m_Node->GetName();
	}

	bool FolderAssetView::IsFolder() {
		return true;
	}

	void FolderAssetView::Open() {
	}

	void FolderAssetView::Save() {
	}

	void FolderAssetView::OnDrag() {
		//ImGui::SetDragDropPayload("Folder", m_Node, sizeof(FolderNode));
	}

	FileAssetView::FileAssetView(FileNode* node): m_Node(node) {
		m_ID = m_Node->GetID();
	}

	const XString& FileAssetView::GetName() {
		return m_Node->GetName();
	}

	bool FileAssetView::IsFolder() {
		return false;
	}

	void FileAssetView::OnDrag() {
		ImGui::SetDragDropPayload("File", m_Node, sizeof(FileNode));
	}

	void MeshAssetView::Open() {
		auto f = [node=m_Node, asset=m_Asset]() {
			for(auto& primitive: asset->Primitives) {

				//material
				File::FPath primitivePath = primitive.BinaryFile;
				const XString primitiveName = File::FPath{ primitive.BinaryFile }.stem().string();
				ImGui::DraggableFileItemAssets(primitiveName.c_str(), primitive.MaterialFile, ".texture");
			}
			if(ImGui::Button("Save")) {
				node->Save();
				EditorLevelMgr::Instance()->ReloadLevel();
			}
		};
		EditorUIMgr::Instance()->AddWindow("MeshViewer", MoveTemp(f), ImGuiWindowFlags_NoDocking);
	}

	void TextureAssetView::Open() {
	}

	void LevelAssetView::Open() {
		EditorLevelMgr::Instance()->LoadLevel(m_Node->GetPathStr().c_str());
	}

	TUniquePtr<AssetViewBase> CreateAssetView(FileNode* node) {
		const XString& ext = node->GetPath().extension().string();
		if (ext == ".mesh") {
			return TUniquePtr(new MeshAssetView(node));
		}
		if (ext == ".texture") {
			return TUniquePtr(new TextureAssetView(node));
		}
		if (ext == ".level") {
			return TUniquePtr(new LevelAssetView(node));
		}
		return TUniquePtr(new FileAssetView(node));
	}
}
