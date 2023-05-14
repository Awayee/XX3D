#include "AssetsWindow.h"
#include "Context/Public/EditorContext.h"

namespace Editor {

	inline AssetBrowser* Browser() { return Context()->GetAssetBrowser(); }

	TVector<AssetsWindow*> AssetsWindow::s_Instances;

	AssetsWindow::NodeItem::NodeItem(const FileNode* file) {
		ID = file->GetID();
		Name = file->GetName().c_str();
		switch (file->GetType()) {
		case EFileType::MESH:
			Type = EDisplayType::MESH; break;
		case EFileType::TEXTURE:
			Type = EDisplayType::TEXTURE; break;
		case EFileType::SCENE:
			Type = EDisplayType::SCENE; break;
		default:
			Type = EDisplayType::UNKNOWN; break;
		}
	}

	AssetsWindow::NodeItem::NodeItem(const FolderNode* folder) {
		ID = folder->GetID();
		Name = folder->GetName().c_str();
		Type = EDisplayType::FOLDER;
	}

	void AssetsWindow::OnFolderRebuildAllWindows(const FolderNode* node) {
		for(AssetsWindow* wnd: s_Instances) {
			wnd->OnFolderRebuild(node);
		}
	}

	void AssetsWindow::OnFolderRebuild(const FolderNode* node) {
		if(m_CurrentFolder && node->Contains(m_CurrentFolder)) {
			RefreshItems();
		}
	}

	void AssetsWindow::RefreshItems() {
		if(!m_CurrentFolder) {
			return;
		}
		m_Contents.clear();
		for(NodeID id : m_CurrentFolder->GetChildFolders()) {
			FolderNode* node = Browser()->GetFolder(id);
			if(node) {
				m_Contents.emplace_back(node);
			}
		}
		for(NodeID id : m_CurrentFolder->GetChildFiles()) {
			FileNode* node = Browser()->GetFile(id);
			if(node) {
				m_Contents.emplace_back(node);
			}

		}
	}

	AssetsWindow::AssetsWindow() : EditorWindowBase("Assets") {
		if(s_Instances.empty()) {
			Browser()->RegisterFolderRebuildEvent(AssetsWindow::OnFolderRebuildAllWindows);
		}
		s_Instances.push_back(this);
		m_CurrentFolder = Browser()->GetRoot();
		RefreshItems();
	}

	AssetsWindow::~AssetsWindow() {
		SwapRemove(s_Instances, this);
	}

	void AssetsWindow::OnWindow() {
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

		for(uint32 i=0; i< m_Contents.size(); ++i) {
			NodeItem& item = m_Contents[i];
			if (ImGui::Selectable(item.Name, m_SelectedItem == i, ImGuiSelectableFlags_AllowDoubleClick)) {
				if(m_SelectedItem == i) {
					if (item.Type == EDisplayType::FOLDER) {
						m_CurrentFolder = Browser()->GetFolder(item.ID);
						RefreshItems();
						break;
					}
					else {
						PRINT("ASSET: %s, %i", item.Name, item.Type);
					}					
				}
				else {
					m_SelectedItem = i;

				}
			}
		}
	}
}
