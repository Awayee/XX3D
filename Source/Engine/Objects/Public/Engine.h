#pragma once
#include "Render/Public/WindowSystem.h"
#include "Render/Public/RenderSystem.h"
#include "Core/Public/Time.h"
#include "Core/Public/SmartPointer.h"

namespace Engine {
	class XXEngine {
	private:
		TUniquePtr<WindowSystemBase> m_Window {nullptr};
		TUniquePtr<RenderSystem> m_Renderer{ nullptr };
	public:
		XXEngine();
		~XXEngine();
		bool Tick();
		WindowSystemBase* Window() const { return m_Window.get(); }
		RenderSystem* Renderer() const { return m_Renderer.get(); }
	};
}