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
		// ------------------mesh-------------------------------
		const char* meshFile = "keqingImported/keqing.mesh";
		m_MeshAsset.reset(new AMeshAsset());

		//import glb
		const char* file = "keqing/keqing.glb";
		PARSE_PROJECT_ASSET(file);
		MeshImporter importer(m_MeshAsset.get(), meshFile);
		if(importer.Import(file) &&
			importer.Save()) {
			PRINT("Save Mesh Successfully!");
		}

		//load asset
		//m_MeshAsset->Load(meshFile);
		m_RenderMesh.reset(new Engine::RenderMesh(m_MeshAsset.get(), Editor::Context()->m_Scene));

		// ---------------------texture---------------------------------
		//const char* textureFile = "keqingImported/tex/cloth.texture";
		//m_TextureAsset.reset(new ATextureAsset);
		//// import png
		//const char* externalTex = "keqing/tex/cloth.png";
		//PARSE_PROJECT_ASSET(externalTex);
		//TextureImporter texImporter(m_TextureAsset.get(), textureFile);
		//if(texImporter.Import(externalTex) &&
		//	texImporter.Save()) {
		//	PRINT("Save Texture Successfully!");
		//}
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
