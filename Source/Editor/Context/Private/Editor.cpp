#include "Context/Public/Editor.h"
#include "Context/Public/EditorContext.h"
#include "Resource/Public/Config.h"
#include "Resource/Public/Assets.h"
#include "Objects/Public/EngineContext.h"

namespace Editor {

	void XXEditor::InitResources(){
		Editor::Context()->m_Scene = Engine::RenderScene::GetDefaultScene();
		Engine::AExternalMeshAsset meshAsset = Engine::Assets::LoadAsset<Engine::AExternalMeshAsset>("keqing/keqing.glb");
		m_Ms.reset(new Engine::RenderMesh(meshAsset.Primitives, nullptr));
	}

	XXEditor::XXEditor(Engine::XXEngine* engine){
		m_Engine = engine;
		m_EditorUI.reset(new UIMgr());
		Engine::Context()->Renderer()->InitUIPass();
		Engine::Context()->Renderer()->SetEnable(true);
		Editor::Context()->m_FPSRecorder.reset(new Engine::FPSRecorder);
		InitResources();
	}

	XXEditor::~XXEditor() {
		Editor::Context()->m_FPSRecorder.reset();
	}

	void XXEditor::EditorRun(){
		while(true) {
			if(!m_Engine->Tick()) {
				return;
			}
			// use ui data on pre-frame
			m_EditorUI->Tick();
			Editor::Context()->FPSRec()->Tick();
		}
	}
}