#include "Objects/Public/EngineContext.h"
#include "Window/Public/Wnd.h"

namespace Engine {

	EngineContext* Context() {
		static EngineContext s_EngineContext;
		return &s_EngineContext;
	}

}