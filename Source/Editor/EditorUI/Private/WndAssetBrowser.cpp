#include "WndAssetBrowser.h"
#include "EditorUI/Public/EditorUIMgr.h"

namespace Editor {

	inline AssetManager* Browser() { return ProjectAssetMgr::Instance(); }

	inline bool IsMouseOnTreeNodeArrow() {
		ImVec2 arrowMin = ImGui::GetItemRectMin();
		ImVec2 arrowMax = ImGui::GetItemRectMax();
		arrowMax.x = arrowMin.x + ImGui::GetTreeNodeToLabelSpacing();
		return ImGui::IsMouseHoveringRect(arrowMin, arrowMax);
	}

	void ImportAsset() {
		auto f = []() {
			static char s_SrcFile[128];
			static char s_DstFile[128];
			ImGui::InputText("Source", s_SrcFile, sizeof(s_SrcFile));
			ImGui::InputText("Target", s_DstFile, sizeof(s_DstFile));
			if(ImGui::Button("Import")) {
				ProjectAssetMgr::Instance()->ImportAsset(s_SrcFile, s_DstFile);
			}
		};
		EditorUIMgr::Instance()->AddWindow("AssetImporter", std::move(f));
	}

	TVector<WndAssetBrowser*> WndAssetBrowser::s_Instances;

	void WndAssetBrowser::OnFolderRebuildAllWindows(const FolderNode* node) {
		for(WndAssetBrowser* wnd: s_Instances) {
			wnd->OnFolderRebuild(node);
		}
	}

	void WndAssetBrowser::OnFolderRebuild(const FolderNode* node) {
		if(m_CurrentFolder && node->Contains(m_CurrentFolder->GetID())) {
			RefreshItems();
		}
	}

	void WndAssetBrowser::RefreshItems() {
		if(!m_CurrentFolder) {
			return;
		}
		m_Contents.Clear();
		for(NodeID id : m_CurrentFolder->GetChildFolders()) {
			FolderNode* node = Browser()->GetFolder(id);
			if(node) {
				m_Contents.PushBack(TUniquePtr<FolderAssetView>(new FolderAssetView(node)));
			}
		}
		for(NodeID id : m_CurrentFolder->GetChildFiles()) {
			FileNode* node = Browser()->GetFile(id);
			if(node) {
				m_Contents.PushBack(CreateAssetView(node));
			}

		}
	}

	void WndAssetBrowser::DisplayFolderTree(NodeID node) {
		FolderNode* folderNode = Browser()->GetFolder(node);
		if(!folderNode) {
			return;
		}
		auto& children = folderNode->GetChildFolders();
		ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow;
		if(children.Empty()) {
			nodeFlags |= ImGuiTreeNodeFlags_Leaf;
		}

		if(folderNode == m_CurrentFolder) {
			nodeFlags |= ImGuiTreeNodeFlags_Selected;
		}

		bool expanded = ImGui::TreeNodeEx(folderNode->GetName().c_str(), nodeFlags);
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
			if (children.Empty() || !IsMouseOnTreeNodeArrow()) {
				SetCurrentFolder(folderNode);
			}
		}

		if(expanded){
			for(NodeID child: children) {
				DisplayFolderTree(child);
			}
			ImGui::TreePop();
		}
	}

	void WndAssetBrowser::DisplayContents() {
		if (m_CurrentFolder->ParentFolder() != INVALLID_NODE) {
			if (ImGui::ArrowButton("Back", ImGuiDir_Left)) {
				m_CurrentFolder = Browser()->GetFolder(m_CurrentFolder->ParentFolder());
				RefreshItems();
				return;
			}
		}
		else {
			ImGui::ArrowButton("Back", ImGuiDir_Left);
		}

		ImGui::BeginChild("Assets");
		for (uint32 i = 0; i < m_Contents.Size(); ++i) {
			auto& item = m_Contents[i];

			if (ImGui::Selectable(item->GetName().c_str(), m_SelectedItem == i, ImGuiSelectableFlags_AllowDoubleClick)) {
				m_SelectedItem = i;
				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
					if (item->IsFolder()) {
						SetCurrentFolder(Browser()->GetFolder(item->m_ID));
					}
					else {
						item->Open();
					}
				}
			}

			if (ImGui::BeginDragDropSource()) {
				ImGui::Text(item->GetName().c_str());
				item->OnDrag();
				ImGui::EndDragDropSource();
			}

			if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
			}
		}
		ImGui::EndChild();
	}

	WndAssetBrowser::WndAssetBrowser() : EditorWndBase("Assets") {
		EditorUIMgr::Instance()->AddMenu("Menu", "Import", ImportAsset, nullptr);
		EditorUIMgr::Instance()->AddMenu("Window", m_Name, {}, &m_Enable);
		if(s_Instances.Empty()) {
			Browser()->RegisterFolderRebuildEvent(WndAssetBrowser::OnFolderRebuildAllWindows);
		}
		s_Instances.PushBack(this);
		SetCurrentFolder(Browser()->GetRoot());
	}

	WndAssetBrowser::~WndAssetBrowser() {
		s_Instances.SwapRemove(this);
	}

	void WndAssetBrowser::Update() {
	}

	void WndAssetBrowser::WndContent() {

		ImGui::Columns(2);

		// folders
		ImGui::SetColumnWidth(0, 256.0f);
		DisplayFolderTree(Browser()->RootID());

		// files
		ImGui::NextColumn();
		DisplayContents();
	}

	void WndAssetBrowser::SetCurrentFolder(FolderNode* node) {
		m_CurrentFolder = node;
		RefreshItems();
	}
}
