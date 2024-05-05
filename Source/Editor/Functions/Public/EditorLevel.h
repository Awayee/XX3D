#pragma once
#include "Asset/Public/LevelAsset.h"
#include "Objects/Public/RenderScene.h"
#include "Objects/Public/StaticMesh.h"
#include "Functions/Public/AssetManager.h"

namespace Editor {
	struct EditorLevelMesh {
		XXString File;
		XXString Name;
		Engine::AMeshAsset* Asset{nullptr};
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
		EditorLevel(const Engine::ALevelAsset& asset, Engine::RenderScene* scene);
		~EditorLevel();
		TVector<EditorLevelMesh>& Meshes();
		EditorLevelMesh* GetMesh(uint32 idx);
		EditorLevelMesh* AddMesh(const XXString& file, Engine::AMeshAsset* asset);
		void DelMesh(uint32 idx);
		void SaveAsset(Engine::ALevelAsset* asset);
	};
}