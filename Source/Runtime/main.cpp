#include "Objects/Public/Engine.h"
#include "Objects/Public/EngineContext.h"
#include "Core/Public/Defines.h"
#include "Resource/Public/Config.h"
#include "Objects/Public/Renderer.h"
#include "ClientCode/Public/Client.h"

int main() {
	// Run Editor
	{
		PRINT("Runtime");
		Engine::XXEngine engine{};
		Runtime::Client client{};
		Engine::Context()->Renderer()->SetRenderArea({ 0, 0, Engine::GetConfig().WindowSize.w, Engine::GetConfig().WindowSize.h });
		while (engine.Tick()) {
			client.Tick();
		}
		Engine::Context()->Renderer()->WaitGPU();
	}
	return 0;
}
