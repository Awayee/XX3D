#include "Objects/Public/EngineContext.h"
#include "Render/Public/WindowSystem.h"
#include "Render/Public/RenderSystem.h"

namespace Engine {

	EngineContext* Context() {
		static EngineContext s_EngineContext;
		return &s_EngineContext;
	}

}