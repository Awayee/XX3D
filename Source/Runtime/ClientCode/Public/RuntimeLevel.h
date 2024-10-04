#pragma once
#include "Core/Public/Defines.h"
#include "Objects/Public/Level.h"

namespace Runtime {
	class RuntimeLevelMgr {
		SINGLETON_INSTANCE(RuntimeLevelMgr);
	public:
	private:
		TUniquePtr<Object::Level> m_Level;
		RuntimeLevelMgr();
		~RuntimeLevelMgr();
	};
}