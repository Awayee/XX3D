#pragma once
#include "Render/Public/RenderScene.h"
#include "Objects/Public/Timer.h"
namespace Editor {

	class EditorContext {
	private:
		friend class XXEditor;
		Engine::RenderScene* m_Scene{nullptr};
		TUniquePtr<Engine::CTimer> m_Timer;
	public:
		Engine::RenderScene* CurrentScene() { return m_Scene; }
		Engine::CTimer* Timer() { return m_Timer.get(); }
	};

	EditorContext* Context();
}
