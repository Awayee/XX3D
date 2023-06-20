#include "Context/Public/EditorSceneManager.h"

namespace Editor {
	Engine::RenderScene* EditorSceneManager::NewScene() {
		m_Scenes.PushBack({});
		m_Scenes.Back().reset(new Engine::RenderScene);
		return m_Scenes.Back().get();
	}

	void EditorSceneManager::RemoveScene(Engine::RenderScene* scene) {
		for(uint32 i=0; i<m_Scenes.Size(); ++i) {
			if(scene == m_Scenes[i].get()) {
				m_Scenes.SwapRemoveAt(i);
				return;
			}
		}
		if(m_DefaultScene == scene) {
			m_DefaultScene = nullptr;
		}
	}

	void EditorSceneManager::Clear() {
		m_Scenes.Clear();
		m_DefaultScene = nullptr;
	}

	void EditorSceneManager::SetDefaultScene(Engine::RenderScene* scene) {
		m_DefaultScene = scene;
	}

}
