#include "Context/Public/Editor.h"
#include "Context/Public/EditorContext.h"
#include "Asset/Public/AssetLoader.h"
#include "Objects/Public/EngineContext.h"


namespace Editor {
	
	XXEditor::XXEditor(Engine::XXEngine* engine){
		m_Engine = engine;
		Editor::Context()->m_AssetBrowser.reset(new AssetManager(PROJECT_ASSETS));

		Editor::Context()->m_SceneMgr.reset(new EditorSceneManager);
		auto scene = Editor::Context()->m_SceneMgr->NewScene();
		Editor::Context()->m_SceneMgr->SetDefaultScene(scene);

		m_EditorUI.reset(new UIMgr());
		Engine::Context()->Renderer()->InitUIPass();
		Engine::Context()->Renderer()->SetEnable(true);
		Editor::Context()->m_Timer.reset(new Engine::CTimer);
	}

	XXEditor::~XXEditor() {
		Editor::Context()->m_Timer.reset();
		Editor::Context()->m_AssetBrowser.reset();
		Editor::Context()->m_SceneMgr.reset();
	}

	void XXEditor::EditorRun(){
		while(true) {
			if(!m_Engine->Tick()) {
				return;
			}
			// use ui data on pre-frame
			m_EditorUI->Tick();
			Editor::Context()->Timer()->Tick();
		}
	}
}
