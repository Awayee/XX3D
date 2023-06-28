#pragma once
#include "Objects/Public/RenderScene.h"
#include "Objects/Public/StaticMesh.h"
#include "Asset/Public/LevelAsset.h"

namespace Engine {
	class Level {
	private:
		RenderScene* m_Scene;
		TVector<TUniquePtr<StaticMesh>> m_Meshes;
	public:
		Level() = default;
		Level(const ALevelAsset& levelAsset, RenderScene* scene);
		~Level();
	};
}