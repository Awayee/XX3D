#pragma once
#include "Objects/Public/RenderScene.h"
#include "Objects/Public/StaticMesh.h"
#include "Asset/Public/LevelAsset.h"

namespace Object {
	class Level {
	private:
		RenderScene* m_Scene;
	public:
		Level() = default;
		Level(const Asset::LevelAsset & levelAsset, RenderScene* scene);
		~Level();
	};
}