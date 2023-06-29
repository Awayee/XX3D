#include "WndAssetBrowser.h"
#include "EditorUI/Public/EditorUIMgr.h"

namespace Editor {

	inline AssetManager* Browser() { return ProjectAssetMgr::Instance(); }

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
				m_Contents.PushBack(MakeUniquePtr<FolderAssetView>(node));
			}
		}
		for(NodeID id : m_CurrentFolder->GetChildFiles()) {
			FileNode* node = Browser()->GetFile(id);
			if(node) {
				m_Contents.PushBack(CreateAssetView(node));
			}

		}
	}

	WndAssetBrowser::WndAssetBrowser() : EditorWindowBase("Assets") {
		EditorUIMgr::Instance()->AddMenu("Menu", "Import", ImportAsset, nullptr);
		EditorUIMgr::Instance()->AddMenu("Window", m_Name, {}, &m_Enable);
		if(s_Instances.Empty()) {
			Browser()->RegisterFolderRebuildEvent(WndAssetBrowser::OnFolderRebuildAllWindows);
		}
		s_Instances.PushBack(this);
		m_CurrentFolder = Browser()->GetRoot();
		RefreshItems();
	}

	WndAssetBrowser::~WndAssetBrowser() {
		s_Instances.SwapRemove(this);
	}

	void WndAssetBrowser::Update() {
	}

	void WndAssetBrowser::Display() {
		if(m_CurrentFolder != Browser()->GetRoot()) {
			if(ImGui::ArrowButton("Back", ImGuiDir_Left)) {
				m_CurrentFolder = Browser()->GetFolder(m_CurrentFolder->ParentFolder());
				RefreshItems();
				return;
			}
		}
		else {
			ImGui::ArrowButton("Back", ImGuiDir_Left);
		}

		ImGui::BeginChild("Assets");
		for(uint32 i=0; i< m_Contents.Size(); ++i) {
			auto& item = m_Contents[i];

			if (ImGui::Selectable(item->m_Name.c_str(), m_SelectedItem == i, ImGuiSelectableFlags_AllowDoubleClick)) {
				m_SelectedItem = i;
				if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
					if(item->IsFolder()) {
						m_CurrentFolder = Browser()->GetFolder(item->m_ID);
						RefreshItems();
					}
					else {
						item->Open();
					}
				}
			}

			if (ImGui::BeginDragDropSource()) {
				ImGui::Text(item->m_Name.c_str());
				item->OnDrag();
				ImGui::EndDragDropSource();
			}
		}
		ImGui::EndChild();

		if(ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
		}
	}
}
