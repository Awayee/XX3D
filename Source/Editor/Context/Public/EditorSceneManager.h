#pragma once
#include "Render/Public/RenderScene.h"

namespace Editor {
	class EditorSceneManager {
	private:
		Engine::RenderScene* m_DefaultScene;
		TVector<TUniquePtr<Engine::RenderScene>> m_Scenes;
	public:
		EditorSceneManager() = default;
		~EditorSceneManager() = default;
		Engine::RenderScene* NewScene();
		void RemoveScene(Engine::RenderScene* scene);
		void Clear();
		void SetDefaultScene(Engine::RenderScene* scene);
		Engine::RenderScene* GetDefaultScene() { return m_DefaultScene; }
	};
}
