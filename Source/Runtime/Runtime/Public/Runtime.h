#pragma once
#include "Engine/Public/Engine.h"

namespace Runtime {
	class XXRuntime: public Engine::XXEngine{
	public:
		XXRuntime();
		~XXRuntime() override;
	private:
		void Update() override;
	};
}