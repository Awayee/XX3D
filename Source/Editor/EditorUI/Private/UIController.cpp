#include "EditorUI/Public/UIController.h"
#include "EditorUI/Public/EditorUIMgr.h"
#include "Objects/Public/EngineContext.h"
#include "Functions/Public/EditorTimer.h"
#include "Window/Public/Wnd.h"
#include "WndViewport.h"
#include "WndAssetBrowser.h"
#include "WndDetails.h"
#include "WndLevelHierarchy.h"

namespace Editor {

	void EditorExit(){
		Engine::Context()->Window()->Close();
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
			[]() {ImGui::Text("FPS = %u", static_cast<uint32>(EditorTimer::Instance()->GetFPS()));},
			ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking);

		//menu items
		uiMgr->AddMenu("Menu", "Exit", EditorExit, nullptr);
	}

	UIController::~UIController() {

	}

	void UIController::Tick() {
	}
}
