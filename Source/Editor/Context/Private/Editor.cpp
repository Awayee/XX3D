#include "Context/Public/Editor.h"
#include "Asset/Public/AssetLoader.h"
#include "Functions/Public/EditorConfig.h"
#include "EditorUI/Public/UIController.h"
#include "EditorUI/Public/EditorUIMgr.h"
#include "Functions/Public/AssetManager.h"
#include "Functions/Public/EditorLevel.h"
#include "RHI/Public/RHIImGui.h"
#include "System/Public/Configuration.h"
#include "Window/Public/EngineWindow.h"

namespace Editor {

	void TestWindowFunc() {
		Engine::EngineWindow* window = Engine::EngineWindow::Instance();
		// TODO test
		window->RegisterOnCursorPosFunc([](float x, float y) {LOG_DEBUG("OnCursorPos(%f, %f)", x, y); });
		window->RegisterOnKeyFunc([](Engine::EKey key, Engine::EInput input) {LOG_DEBUG("OnKey(%i, %i)", key, input); });
		window->RegisterOnMouseButtonFunc([](Engine::EBtn btn, Engine::EInput input) {LOG_DEBUG("OnMouseButton(%i, %i)", btn, input); });
		window->RegisterOnScrollFunc([](float x, float y) {LOG_DEBUG("OnScroll(%f,%f)", x, y); });
		window->RegisterOnWindowFocusFunc([](bool b) {LOG_DEBUG("OnWindowFocus(%i)", b); });
		window->RegisterOnDropFunc([](int num, const char** path) {
			LOG_DEBUG("OnDrop(%i)", num);
			for (int i = 0; i < num; ++i) {
				LOG_DEBUG("    Path=%s", path[i]);
			}
			});
	}

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
		RHI::SetInitSetupFunc([](RHIInitConfig& cfg){
			cfg.EnableVSync = true;
		});
		Engine::EngineWindow::SetInitSetupFunc([](Engine::WindowInitInfo& info) {
			info.Resizeable = true;
			info.Title = StringFormat("%s XXEditor", info.Title.c_str());
		});
		//clear all compiled shader cache, to recompile them.
		File::RemoveDir(Engine::EngineConfig::Instance().GetCompiledShaderDir());
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
