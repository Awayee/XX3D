#include "WndLevelHierarchical.h"
#include "Functions/Public/EditorLevelMgr.h"
#include "EditorUI/Public/EditorUIMgr.h"
#include "Functions/Public/EditorLevelMgr.h"
#include "Asset/Public/AssetLoader.h"

namespace Editor {
	void SaveLevel() {
		if (!EditorLevelMgr::Instance()->SaveLevel()) {
			static EditorWindowBase* s_FileSelectWnd = nullptr;
			auto f = []() {
				static char s_FilePath[128] = { 0 };
				bool close = false;
				ImGui::InputText("FilePath", s_FilePath, sizeof(s_FilePath));
				if (ImGui::Button("Save")) {
					if (StrEmpty(s_FilePath)) {
						LOG("Invalid file path!");
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
					EditorUIMgr::Instance()->DeleteWindow(s_FileSelectWnd);
				}
			};
			s_FileSelectWnd = EditorUIMgr::Instance()->AddWindow("Select a Path", std::move(f), ImGuiWindowFlags_NoDocking);
		}
	}

	void WndLevelHierarchical::Update() {
	}

	void WndLevelHierarchical::Display() {
		EditorLevel* level = EditorLevelMgr::Instance()->GetLevel();
		if(!level) {
			return;
		}
		for (uint32 i = 0; i < level->Meshes().Size(); ++i) {
			auto meshInfo = level->GetMesh(i);
			if (ImGui::Selectable(meshInfo->Name.c_str(), m_SelectIdx == i)) {
				m_SelectIdx = i;
				EditorLevelMgr::Instance()->SetSelected(m_SelectIdx);
			}
		}
	}

	WndLevelHierarchical::WndLevelHierarchical(): EditorWindowBase("Hierarchical") {
		EditorUIMgr::Instance()->AddMenu("Level", "Save", SaveLevel, nullptr);
	}

	WndLevelHierarchical::~WndLevelHierarchical() {
	}
}
