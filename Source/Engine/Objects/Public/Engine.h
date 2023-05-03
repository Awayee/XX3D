#pragma once
#include "Window/Public/Wnd.h"
#include "Render/Public/RenderSystem.h"
#include "Core/Public/Time.h"
#include "Core/Public/SmartPointer.h"

namespace Engine {
	class XXEngine {
	private:
	public:
		XXEngine();
		~XXEngine();
		bool Tick();
	};
}