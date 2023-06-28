#include "Context/Public/Editor.h"
#include "Asset/Public/AssetLoader.h"
#include "Objects/Public/EngineContext.h"
#include "Resource/Public/Config.h"

#include "EditorUI/Public/UIController.h"
#include "EditorUI/Public/EditorUIMgr.h"
#include "Functions/Public/AssetManager.h"
#include "Functions/Public/EditorLevelMgr.h"
#include "Functions/Public/EditorTimer.h"
#include "Window/Public/Wnd.h"
#include "Objects/Public/Renderer.h"

namespace Editor {
    void SetUIColorStyle()
    {
		ImGuiStyle& style = ImGui::GetStyle();
    }

	void ImGuiConfig() {
		// imgui
		float scaleX, scaleY;
		Engine::Context()->Window()->GetWindowContentScale(&scaleX, &scaleY);
		float contentScale = fmaxf(1.0f, fmaxf(scaleX, scaleY));

		// load font for imgui
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigWindowsMoveFromTitleBarOnly = true;

		File::FPath fontPath = Engine::AssetLoader::AssetPath();
		fontPath.append(Engine::GetConfig().DefaultFontPath);

		io.Fonts->AddFontFromFileTTF(fontPath.string().c_str(), contentScale * 16, nullptr, nullptr);
		ASSERT(io.Fonts->Build(), "Failed to build fonts");
		//io.IniFilename = nullptr; // Do not save settings

		SetUIColorStyle();
	}
	
	XXEditor::XXEditor(Engine::XXEngine* engine){
		m_Engine = engine;
		Engine::Context()->Renderer()->InitUIPass(ImGuiConfig);
		Editor::EditorTimer::Initialize();
		EngineAssetMgr::Initialize();
		ProjectAssetMgr::Initialize();
		EditorUIMgr::Initialize();
		EditorLevelMgr::Initialize();
		m_UIController.reset(new UIController);
	}

	XXEditor::~XXEditor() {
		Engine::RHI::Instance()->WaitGraphicsQueue();//wait last submitted commands
		Editor::EditorTimer::Release();
		EngineAssetMgr::Release();
		ProjectAssetMgr::Release();
		EditorUIMgr::Release();
		EditorLevelMgr::Release();
	}

	void XXEditor::EditorRun(){

		while (true) {
			EditorTimer::Instance()->Tick();
			EditorUIMgr::Instance()->Tick();
			if (!m_Engine->Tick()) {
				return;
			}
		}
	}
}
