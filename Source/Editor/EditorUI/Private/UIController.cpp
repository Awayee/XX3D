#include "EditorUI/Public/UIController.h"
#include "EditorUI/Public/EditorUIMgr.h"
#include "Functions/Public/EditorConfig.h"
#include "Window/Public/EngineWindow.h"
#include "System/Public/Timer.h"
#include "WndViewport.h"
#include "WndAssetBrowser.h"
#include "WndDetails.h"
#include "WndLevelHierarchy.h"

namespace Editor {

	void EditorExit(){
		Engine::EngineWindow::Instance()->Close();
	}

	void SaveLayout() {
		ImGui::SaveIniSettingsToDisk(EditorConfig::Instance()->GetImGuiConfigSavePath().c_str());
	}

	UIController::UIController() {
		EditorUIMgr* uiMgr = EditorUIMgr::Instance();

		uiMgr->AddMenuBar("Menu");
		uiMgr->AddMenuBar("Level");
		uiMgr->AddMenuBar("Window");

		//windows
		uiMgr->AddWindow(TUniquePtr(new WndAssetBrowser()));
		uiMgr->AddWindow(TUniquePtr(new WndViewport()));
		uiMgr->AddWindow(TUniquePtr(new WndLevelHierarchy()));
		uiMgr->AddWindow(TUniquePtr(new WndDetails()));
		uiMgr->AddWindow("FPS",
			[]() {ImGui::Text("FPS = %u", static_cast<uint32>(Engine::CTimer::Instance()->GetFPS())); },
			ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground);

		//menu items
		uiMgr->AddMenu("Menu", "Save Layout", SaveLayout, nullptr);
		uiMgr->AddMenu("Menu", "Exit", EditorExit, nullptr);
	}

	UIController::~UIController() {

	}

	void UIController::Tick() {
	}
}
