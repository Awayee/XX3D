#pragma once
#include "Render/Public/RenderScene.h"
#include "Objects/Public/FPSRecorder.h"
namespace Editor {

	class EditorContext {
	private:
		friend class XXEditor;
		Engine::RenderScene* m_Scene{nullptr};
		TUniquePtr<Engine::FPSRecorder> m_FPSRecorder;
	public:
		Engine::RenderScene* CurrentScene() { return m_Scene; }
		Engine::FPSRecorder* FPSRec() { return m_FPSRecorder.get(); }
	};

	EditorContext* Context();
}
