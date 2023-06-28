#include "Objects/Public/EngineContext.h"
#include "Window/Public/Wnd.h"
#include "Objects/Public/Renderer.h"

namespace Engine {

	EngineContext* Context() {
		static EngineContext s_EngineContext;
		return &s_EngineContext;
	}

}