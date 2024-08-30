#pragma once
#include "Objects/Public/RenderScene.h"
#include "Asset/Public/LevelAsset.h"

namespace Object {
	class Level {
	public:
		Level() = default;
		Level(const Asset::LevelAsset& levelAsset, RenderScene* scene);
		~Level();
	protected:
		RenderScene* m_Scene;
	};
}