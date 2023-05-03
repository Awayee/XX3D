#include "Objects/Public/EngineContext.h"
#include "Window/Public/Wnd.h"
#include "Render/Public/RenderSystem.h"

namespace Engine {

	EngineContext* Context() {
		static EngineContext s_EngineContext;
		return &s_EngineContext;
	}

}