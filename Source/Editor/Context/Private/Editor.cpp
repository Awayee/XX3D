#include "Context/Public/Editor.h"
#include "Asset/Public/AssetLoader.h"
#include "Objects/Public/EngineContext.h"
#include "Resource/Public/Config.h"
#include "EditorUI/Public/UIMgr.h"
#include "Functions/Public/AssetManager.h"
#include "Functions/Public/EditorLevelMgr.h"
#include "Functions/Public/EditorTimer.h"


namespace Editor {
	void ImGuiConfig() {
		// imgui
		float scaleX, scaleY;
		Engine::Context()->Window()->GetWindowContentScale(&scaleX, &scaleY);
		float contentScale = fmaxf(1.0f, fmaxf(scaleX, scaleY));

		// load font for imgui
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigWindowsMoveFromTitleBarOnly = true;

		File::FPath fontPath = AssetLoader::AssetPath();
		fontPath.append(Engine::GetConfig().DefaultFontPath);

		io.Fonts->AddFontFromFileTTF(fontPath.string().c_str(), contentScale * 16, nullptr, nullptr);
		ASSERT(io.Fonts->Build(), "Failed to build fonts");
		//io.IniFilename = nullptr; // Do not save settings

	}
	
	XXEditor::XXEditor(Engine::XXEngine* engine){
		m_Engine = engine;
		Engine::Context()->Renderer()->InitUIPass(ImGuiConfig);
		Editor::EditorTimer::Initialize();
		EngineAssetMgr::Initialize();
		ProjectAssetMgr::Initialize();
		UIMgr::Initialize();
		EditorLevelMgr::Initialize();
	}

	XXEditor::~XXEditor() {
		Editor::EditorTimer::Release();
		EngineAssetMgr::Release();
		ProjectAssetMgr::Release();
		UIMgr::Release();
		EditorLevelMgr::Release();
	}

	void XXEditor::EditorRun(){

		while (true) {
			UIMgr::Instance()->Tick();
			if (!m_Engine->Tick()) {
				return;
			}
		}
	}
}
