#pragma once
#include "Core/Public/SmartPointer.h"

namespace Engine {
	class Wnd;
}

namespace Engine {

	class RenderSystem;

	class EngineContext {
	private:
		friend class XXEngine;
		Engine::Wnd* m_Window;
		TUniquePtr<RenderSystem> m_Renderer;
	public:
		Engine::Wnd* Window() { return m_Window; }
		RenderSystem* Renderer() { return m_Renderer.get(); }
	};

	EngineContext* Context();
}