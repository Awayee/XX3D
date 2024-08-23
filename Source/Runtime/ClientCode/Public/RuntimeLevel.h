#pragma once
#include "Objects/Public/Level.h"

namespace Runtime {
	class RuntimeLevel: public Object::Level {
	public:
		RuntimeLevel(const Asset::LevelAsset& asset, Object::RenderScene* scene);
	};
}