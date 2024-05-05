#pragma once
#include "Objects/Public/Level.h"

namespace Runtime {
	class Client {
	private:
		TUniquePtr<Engine::Level> m_Level;
	public:
		Client();
		void Tick();
		~Client();
	};
}