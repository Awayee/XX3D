#include "ClientCode/Public/RuntimeLevel.h"
#include "System/Public/EngineConfig.h"

namespace Runtime {
	RuntimeLevelMgr::RuntimeLevelMgr() {
		m_Level.Reset(new Object::Level(Object::RenderScene::GetDefaultScene()));
		auto& levelFile = Engine::ConfigManager::GetData().StartLevel;
		m_Level->LoadFile(levelFile.c_str());
	}

	RuntimeLevelMgr::~RuntimeLevelMgr() {
	}
}
