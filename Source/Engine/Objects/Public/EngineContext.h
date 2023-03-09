#pragma once
#include "Core/Public/SmartPointer.h"
namespace Engine {

	class WindowSystemBase;
	class RenderSystem;

	class EngineContext {
	private:
		friend class XXEngine;
		TUniquePtr<WindowSystemBase> m_Window;
		TUniquePtr<RenderSystem> m_Renderer;
	public:
		WindowSystemBase* Window() { return m_Window.get(); }
		RenderSystem* Renderer() { return m_Renderer.get(); }
	};

	EngineContext* Context();
}