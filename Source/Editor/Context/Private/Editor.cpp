#include "Context/Public/Editor.h"
#include "Context/Public/EditorContext.h"
#include "Resource/Public/Config.h"
#include "Asset/Public/AssetMgr.h"
#include "Asset/Public/MeshAsset.h"
#include "Objects/Public/EngineContext.h"


namespace Editor {

	void XXEditor::InitResources(){
		Editor::Context()->m_Scene = Engine::RenderScene::GetDefaultScene();

		// TODO for test
		const char* meshFile = "keqingImported/keqing.mesh";
		m_MeshAsset.reset(new AMeshAsset(meshFile));

		//import glb
		const char* file = "keqing/keqing.glb";
		PARSE_PROJECT_ASSET(file);
		AMeshImporter importer(m_MeshAsset.get());
		importer.Import(file);
		importer.SavePrimitives();
		m_MeshAsset->Save();

		//load asset
		//m_MeshAsset->Load("KeqingImported/keqing.mesh");
		//m_RenderMesh.reset(new Engine::RenderMesh(m_MeshAsset.get(), Editor::Context()->m_Scene));

		m_RenderMesh.reset(new Engine::RenderMesh(m_MeshAsset.get(), Editor::Context()->m_Scene));

	}

	XXEditor::XXEditor(Engine::XXEngine* engine){
		m_Engine = engine;
		m_EditorUI.reset(new UIMgr());
		Engine::Context()->Renderer()->InitUIPass();
		Engine::Context()->Renderer()->SetEnable(true);
		Editor::Context()->m_Timer.reset(new Engine::CTimer);
		InitResources();
	}

	XXEditor::~XXEditor() {
		Editor::Context()->m_Timer.reset();
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
