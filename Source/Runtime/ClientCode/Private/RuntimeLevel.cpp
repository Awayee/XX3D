#include "ClientCode/Public/RuntimeLevel.h"
#include "System/Public/ConfigManager.h"

namespace Runtime {
	RuntimeLevelMgr::RuntimeLevelMgr() {
		m_Level.Reset(new Object::Level(Object::RenderScene::GetDefaultScene()));
		auto& levelFile = Engine::ConfigMgr::Instance().GetProjectConfig().StartLevel;
		m_Level->LoadFile(levelFile.c_str());
	}

	RuntimeLevelMgr::~RuntimeLevelMgr() {
	}
}
