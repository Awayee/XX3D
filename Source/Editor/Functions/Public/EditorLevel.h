#pragma once
#include "Asset/Public/LevelAsset.h"
#include "Objects/Public/RenderScene.h"
#include "Objects/Public/StaticMesh.h"
#include "Functions/Public/AssetManager.h"

namespace Editor {
	struct EditorLevelMesh {
		XString File;
		XString Name;
		Asset::MeshAsset* Asset;
		TUniquePtr<Object::StaticMesh> Mesh;
		Math::FVector3 Position;
		Math::FVector3 Scale;
		Math::FVector3 Rotation;
	};

	class EditorLevel {
	private:
		TVector<EditorLevelMesh> m_Meshes;
		Object::RenderScene* m_Scene{nullptr};
	public:
		EditorLevel(const Asset::LevelAsset& asset, Object::RenderScene* scene);
		~EditorLevel();
		TVector<EditorLevelMesh>& Meshes();
		EditorLevelMesh* GetMesh(uint32 idx);
		EditorLevelMesh* AddMesh(const XString& file, Asset::MeshAsset* asset);
		void DelMesh(uint32 idx);
		void SaveAsset(Asset::LevelAsset* asset);
	};
}