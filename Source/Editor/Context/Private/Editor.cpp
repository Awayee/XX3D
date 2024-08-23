#include "Context/Public/Editor.h"
#include "Asset/Public/AssetLoader.h"
#include "Functions/Public/EditorConfig.h"
#include "EditorUI/Public/UIController.h"
#include "EditorUI/Public/EditorUIMgr.h"
#include "Functions/Public/AssetManager.h"
#include "Functions/Public/EditorLevelMgr.h"
#include "RHI/Public/ImGuiRHI.h"

namespace Editor {
	XXEditor::XXEditor(): XXEngine() {
		EngineAssetMgr::Initialize();
		ProjectAssetMgr::Initialize();
		EditorConfig::Initialize();
		EditorLevelMgr::Initialize();
		EditorUIMgr::Initialize();
		ImGuiRHI::Initialize(Editor::EditorConfig::InitializeImGuiConfig);
		m_UIController.Reset(new UIController);
	}

	XXEditor::~XXEditor() {
		Render::ViewportRenderer::Instance()->WaitAllFence();
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
