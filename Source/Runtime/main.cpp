#include "Core/Public/Log.h"
#include "System/Public/Config.h"
#include "ClientCode/Public/Client.h"
#include "Engine/Public/Engine.h"

int main() {
	// Run Editor
	{
		LOG_INFO("Runtime");
		Engine::XXEngine engine{};
		Runtime::Client client{};
		engine.Run();
		client.Tick();
	}
	return 0;
}
