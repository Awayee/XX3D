#include "WndLevelHierarchy.h"
#include "Functions/Public/EditorLevel.h"
#include "EditorUI/Public/EditorUIMgr.h"
#include "Core/Public/String.h"
#include "Functions/Public/AssetManager.h"
#include "Objects/Public/StaticMesh.h"

namespace Editor {
	void SaveLevel() {
		const XString& levelFile = EditorLevelMgr::Instance()->GetLevelFile();
		if(levelFile.empty()) {
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
		else {
			EditorLevelMgr::Instance()->SaveLevel();
		}
	}

	WndLevelHierarchy::WndLevelHierarchy() : EditorWndBase("Hierarchy") {
		EditorUIMgr::Instance()->AddMenu("Level", "Save Level", SaveLevel, nullptr);
		EditorUIMgr::Instance()->AddMenu("Window", m_Name, {}, &m_Enable);
	}

	WndLevelHierarchy::~WndLevelHierarchy() {
	}

	void WndLevelHierarchy::Update() {
	}

	void WndLevelHierarchy::WndContent() {
		EditorLevelMgr* levelMgr = EditorLevelMgr::Instance();
		EditorLevel* level = levelMgr->GetLevel();
		if(!level) {
			return;
		}
		uint32 selectedActorIdx = levelMgr->GetSelected();
		for (uint32 i = 0; i < level->GetActorSize(); ++i) {
			Object::LevelActor* actor = level->GetActor(i);
			ImGui::PushID(static_cast<int>(i));
			if(m_RenamingIdx == i) {
				if(ImGui::InputText("##", m_ActorName.Data(), m_ActorName.Size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
					actor->SetName(m_ActorName.Data());
					m_RenamingIdx = INVALID_INDEX;
				}
			}
			else if (ImGui::Selectable(actor->GetName().c_str(), selectedActorIdx == i)) {
				m_RenamingIdx = INVALID_INDEX;
				selectedActorIdx = i;
				levelMgr->SetSelected(i);
			}
			ImGui::PopID();
			//right click
			if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
				m_EditActorIdx = i;
			}
		}
		if(ImGui::BeginPopupContextWindow("Hierarchy_ContextMenu", ImGuiPopupFlags_MouseButtonRight)) {
			m_RenamingIdx = INVALID_INDEX;
			if(m_EditActorIdx != INVALID_INDEX) {
				Object::LevelActor* actor = level->GetActor(m_EditActorIdx);
				if (ImGui::MenuItem("Rename")) {
					m_RenamingIdx = m_EditActorIdx;
					StrCopy(m_ActorName.Data(), actor->GetName().c_str());
				}
				if (ImGui::MenuItem("Delete")) {
					levelMgr->GetLevel()->RemoveActor(actor);
					if (levelMgr->GetSelected() == m_EditActorIdx) {
						levelMgr->SetSelected(INVALID_INDEX);
					}
				}
			}
			else if (ImGui::MenuItem("New")) {
				levelMgr->GetLevel()->AddActor("NewActor");
			}
			ImGui::EndPopup();
		}
		else {
			m_EditActorIdx = INVALID_INDEX;
		}
	}
}
