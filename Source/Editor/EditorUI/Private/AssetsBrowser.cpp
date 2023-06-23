#include "AssetsBrowser.h"

namespace Editor {

	inline AssetManager* Browser() { return ProjectAssetMgr::Instance(); }

	TVector<AssetsBrowser*> AssetsBrowser::s_Instances;

	void AssetsBrowser::OnFolderRebuildAllWindows(const FolderNode* node) {
		for(AssetsBrowser* wnd: s_Instances) {
			wnd->OnFolderRebuild(node);
		}
	}

	void AssetsBrowser::OnFolderRebuild(const FolderNode* node) {
		if(m_CurrentFolder && node->Contains(m_CurrentFolder->GetID())) {
			RefreshItems();
		}
	}

	void AssetsBrowser::RefreshItems() {
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

	AssetsBrowser::AssetsBrowser() : EditorWndBase("Assets") {
		if(s_Instances.Empty()) {
			Browser()->RegisterFolderRebuildEvent(AssetsBrowser::OnFolderRebuildAllWindows);
		}
		s_Instances.PushBack(this);
		m_CurrentFolder = Browser()->GetRoot();
		RefreshItems();
	}

	AssetsBrowser::~AssetsBrowser() {
		s_Instances.SwapRemove(this);
	}

	void AssetsBrowser::Update() {
	}

	void AssetsBrowser::Display() {
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
			if (ImGui::Selectable(item->Name().c_str(), m_SelectedItem == i, ImGuiSelectableFlags_AllowDoubleClick)) {
				if(m_SelectedItem == i) {
					if (item->IsFolder()) {
						m_CurrentFolder = Browser()->GetFolder(item->ID());
						RefreshItems();
						break;
					}
					else {
						item->Open();
					}					
				}
				else {
					m_SelectedItem = i;

				}
			}
		}
		ImGui::EndChild();

		if(ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {

		}
	}
}
