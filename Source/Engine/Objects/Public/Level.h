#pragma once
#include "Render/Public/RenderScene.h"
#include "Render/Public/StaticMesh.h"
#include "Asset/Public/LevelAsset.h"

namespace Engine {
	class Level {
	private:
		RenderScene* m_Scene;
		TVector<StaticMesh> m_Meshes;
	public:
		Level() = default;
		Level(const ALevelAsset& levelAsset, RenderScene* scene);
		~Level();
	};
}