#pragma once
#include "Core/Public/Defines.h"
#include "Objects/Public/Level.h"

namespace Runtime {
	class RuntimeLevel: public Object::Level {
	public:
		RuntimeLevel(const Asset::LevelAsset& asset, Object::RenderScene* scene);
	};

	class RuntimeLevelMgr {
		SINGLETON_INSTANCE(RuntimeLevelMgr);
	public:
	private:
		TUniquePtr<RuntimeLevel> m_Level;

		RuntimeLevelMgr();
		~RuntimeLevelMgr();
	};
}