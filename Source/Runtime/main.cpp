#include "Core/Public/Log.h"
#include "System/Public/EngineConfig.h"
#include "Engine/Public/Engine.h"
#include "Runtime/Public/Runtime.h"

int main() {
	// Run Editor
	{
		LOG_INFO("Runtime");
		Runtime::XXRuntime::PreInitialize();
		Runtime::XXRuntime engine{};
		engine.Run();
	}
	return 0;
}
