#include "Context/Public/Editor.h"
#include "Asset/Public/AssetLoader.h"
#include "Functions/Public/EditorConfig.h"
#include "EditorUI/Public/UIController.h"
#include "EditorUI/Public/EditorUIMgr.h"
#include "Functions/Public/AssetManager.h"
#include "Functions/Public/EditorLevel.h"
#include "RHI/Public/RHIImGui.h"

namespace Editor {
	class EditorSceneRenderer: public Render::ISceneRenderer {
	public:
		void Render(Render::RenderGraph& rg, Render::RGTextureNode* backBufferNode) override {
			// get back buffer size
			const auto& backBufferDesc = backBufferNode->GetRHI()->GetDesc();
			const Rect renderArea = { 0,0, backBufferDesc.Width, backBufferDesc.Height };

			Render::RGRenderNode* imGuiNode = rg.CreateRenderNode("ImGui");

			// render DefaultScene to scene Texture
			Object::RenderScene* defaultScene = Object::RenderScene::GetDefaultScene();
			RHITexture* sceneTexture = EditorUIMgr::Instance()->GetSceneTexture();
			if(defaultScene && sceneTexture) {
				Render::RGTextureNode* sceneTextureNode = rg.CreateTextureNode(sceneTexture, "SceneTexture");
				defaultScene->Render(rg, sceneTextureNode);
				imGuiNode->ReadSRV(sceneTextureNode);
			}
			// render ImGui
			imGuiNode->WriteColorTarget(backBufferNode, 0);
			imGuiNode->SetRenderArea(renderArea);
			imGuiNode->SetTask([this](RHICommandBuffer* cmd) { RHIImGui::Instance()->RenderDrawData(cmd); });
		}
	};

	void XXEditor::PreInitialize() {
		RHI::SetInitConfigBuilder([]()->RHIInitConfig {
			RHIInitConfig cfg{};
			cfg.EnableVSync = true;
			return cfg;
		});
	}

	XXEditor::XXEditor(): XXEngine() {
		Render::Renderer::Instance()->SetSceneRenderer<EditorSceneRenderer>();
		EngineAssetMgr::Initialize();
		ProjectAssetMgr::Initialize();
		EditorLevelMgr::Initialize();
		EditorUIMgr::Initialize();
		RHIImGui::Initialize(Editor::UIController::InitializeImGuiConfig);
		m_UIController.Reset(new UIController);
	}

	XXEditor::~XXEditor() {
		Render::Renderer::Instance()->WaitAllFence();
		EngineAssetMgr::Release();
		ProjectAssetMgr::Release();
		EditorUIMgr::Release();
		EditorLevelMgr::Release();
	}

	void XXEditor::Update() {
		EditorUIMgr::Instance()->Tick();
		XXEngine::Update();
	}
}
