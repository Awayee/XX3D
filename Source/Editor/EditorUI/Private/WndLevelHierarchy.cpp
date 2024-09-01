#include "WndLevelHierarchy.h"
#include "Functions/Public/EditorLevelMgr.h"
#include "EditorUI/Public/EditorUIMgr.h"
#include "Functions/Public/EditorLevelMgr.h"
#include "Asset/Public/AssetLoader.h"
#include "Core/Public/String.h"

namespace Editor {
	void SaveLevel() {
		if (!EditorLevelMgr::Instance()->SaveLevel()) {
			static EditorWndBase* s_FileSelectWnd = nullptr;
			auto f = []() {
				static char s_FilePath[128] = { 0 };
				bool close = false;
				ImGui::InputText("FilePath", s_FilePath, sizeof(s_FilePath));
				if (ImGui::Button("Save")) {
					if (StrEmpty(s_FilePath)) {
						LOG_INFO("Invalid file path!");
					}
					else {
						if (!StrEndsWith(s_FilePath, ".level")) {
							StrAppend(s_FilePath, ".level");
						}
						EditorLevelMgr::Instance()->SetLevelPath(s_FilePath);
						EditorLevelMgr::Instance()->SaveLevel();
						close = true;
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel")) {
					close = true;
				}

				if (close) {
					s_FileSelectWnd->Delete();
					s_FileSelectWnd = nullptr;
				}
			};
			s_FileSelectWnd = EditorUIMgr::Instance()->AddWindow("Select a Path", MoveTemp(f), ImGuiWindowFlags_NoDocking);
		}
	}

	void WndLevelHierarchy::Update() {
	}

	void WndLevelHierarchy::WndContent() {
		EditorLevelMgr* levelMgr = EditorLevelMgr::Instance();
		EditorLevel* level = levelMgr->GetLevel();
		if(!level) {
			return;
		}
		auto startPos = ImGui::GetCursorPos();
		ImGui::Dummy(ImGui::GetWindowSize());
		if(ImGui::BeginDragDropTarget()) {
			ImGui::Button("Add");
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("File")) {
				ASSERT(payload->DataSize == sizeof(FileNode), "");
				FileNode* fileNode = reinterpret_cast<FileNode*>(payload->Data);
				if(fileNode->GetExt() == ".mesh") {
					level->AddMesh(fileNode->GetPathStr(), fileNode->GetAsset<Asset::MeshAsset>());
				}
			}
			ImGui::EndDragDropTarget();
		}
		else {
			ImGui::SetCursorPos(startPos);
			for (uint32 i = 0; i < level->Meshes().Size(); ++i) {
				auto meshInfo = level->GetMesh(i);
				ImGui::PushID(static_cast<int>(i));
				if (ImGui::Selectable(meshInfo->Name.c_str(), m_SelectIdx == i)) {
					m_SelectIdx = i;
					levelMgr->SetSelected(m_SelectIdx);
				}
				ImGui::PopID();
				//right click
				if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(1)) {
					m_PressedIdx = i;
				}
				if(m_PressedIdx == i) {
					if(ImGui::BeginPopupContextItem("Popup")) {
						if(ImGui::MenuItem("Delete")) {
							levelMgr->GetLevel()->DelMesh(i);
							if(levelMgr->GetSelected() == i) {
								levelMgr->SetSelected(INVALID_INDEX);
							}
							m_PressedIdx = INVALID_INDEX;
							m_SelectIdx = INVALID_INDEX;
						}
						ImGui::EndPopup();
					}
				}
			}
		}
	}

	WndLevelHierarchy::WndLevelHierarchy(): EditorWndBase("Hierarchy") {
		EditorUIMgr::Instance()->AddMenu("Level", "Save Level", SaveLevel, nullptr);
		EditorUIMgr::Instance()->AddMenu("Window", m_Name.c_str(), {}, &m_Enable);
	}

	WndLevelHierarchy::~WndLevelHierarchy() {
	}
}
