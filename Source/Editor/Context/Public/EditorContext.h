#pragma once
#include "EditorSceneManager.h"
#include "Render/Public/RenderScene.h"
#include "Objects/Public/Timer.h"
#include "EditorAsset/Public/AssetManager.h"

namespace Editor {

	class EditorContext {
	private:
		friend class XXEditor;
		TUniquePtr<Engine::CTimer> m_Timer;
		TUniquePtr<Editor::AssetManager> m_AssetBrowser;
		TUniquePtr<EditorSceneManager> m_SceneMgr;
	public:
		Engine::RenderScene* DefaultScene() { return m_SceneMgr->GetDefaultScene(); };
		Engine::CTimer* Timer() { return m_Timer.get(); }
		Editor::AssetManager* GetAssetBrowser() { return m_AssetBrowser.get(); }
	};

	EditorContext* Context();
}
