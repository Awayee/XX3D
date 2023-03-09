#include "Objects/Public/Engine.h"
#include "Objects/Public/EngineContext.h"
#include "Core/Public/macro.h"
#include "Resource/Public/Config.h"

int main() {
	// Run Editor
	{
		LOG("Runtime");
		Engine::XXEngine engine;
		Engine::Context()->Renderer()->SetRenderArea({ 0, 0, Engine::GetConfig()->GetWindowSize().w, Engine::GetConfig()->GetWindowSize().h });
		while (engine.Tick());
	}
	return 0;
}
