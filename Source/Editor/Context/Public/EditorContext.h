#pragma once
#include "Render/Public/RenderScene.h"
#include "Objects/Public/Timer.h"
#include "EditorAsset/Public/AssetBrowser.h"

namespace Editor {

	class EditorContext {
	private:
		friend class XXEditor;
		Editor::XXEditor* m_Editor{ nullptr };
		Engine::RenderScene* m_Scene{nullptr};
		TUniquePtr<Engine::CTimer> m_Timer;
		TUniquePtr<Editor::AssetBrowser> m_AssetBrowser;
	public:
		Editor::XXEditor* GetEditor() { return m_Editor; }
		Engine::RenderScene* CurrentScene() { return m_Scene; }
		Engine::CTimer* Timer() { return m_Timer.get(); }
		Editor::AssetBrowser* GetAssetBrowser() { return m_AssetBrowser.get(); }
	};

	EditorContext* Context();
}
