#pragma once
#include "Asset/Public/LevelAsset.h"
#include "Render/Public/RenderScene.h"
#include "Render/Public/StaticMesh.h"
#include "Functions/Public/AssetManager.h"

namespace Editor {
	struct EditorLevelMesh {
		File::FPath File;
		String Name;
		AMeshAsset* Asset{nullptr};
		TUniquePtr<Engine::StaticMesh> Mesh;
		Math::FVector3 Position;
		Math::FVector3 Scale;
		Math::FVector3 Rotation;
	};

	class EditorLevel {
	private:
		TVector<EditorLevelMesh> m_Meshes;
		Engine::RenderScene* m_Scene{nullptr};
	public:
		EditorLevel(const ALevelAsset& asset, Engine::RenderScene* scene);
		~EditorLevel();
		TVector<EditorLevelMesh>& Meshes();
		EditorLevelMesh* GetMesh(uint32 idx);
		EditorLevelMesh* AddMesh(const File::FPath& file, AMeshAsset* asset);
		void SaveAsset(ALevelAsset* asset);
	};
}