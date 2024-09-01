#pragma once
#include "Asset/Public/LevelAsset.h"
#include "Objects/Public/RenderScene.h"
#include "Objects/Public/StaticMesh.h"
#include "Functions/Public/AssetManager.h"
#include "Objects/Public/Level.h"

namespace Editor {
	struct EditorLevelMesh {
		XString File;
		XString Name;
		Asset::MeshAsset* Asset;
		Math::FVector3 Position;
		Math::FVector3 Scale;
		Math::FVector3 Rotation;
		Object::EntityID ObjectEntityID;
	};

	class EditorLevel: public Object::Level{
	public:
		EditorLevel(const Asset::LevelAsset& asset, Object::RenderScene* scene);
		~EditorLevel();
		Object::RenderScene* GetScene();
		TArray<EditorLevelMesh>& Meshes();
		EditorLevelMesh* GetMesh(uint32 idx);
		EditorLevelMesh* AddMesh(const XString& file, Asset::MeshAsset* asset);
		void DelMesh(uint32 idx);
		void SaveAsset(Asset::LevelAsset* asset);
		const XString& GetSkyBoxFile() { return m_SkyBoxFile; }
		void SetSkyBoxFile(const XString& file);
	private:
		TArray<EditorLevelMesh> m_Meshes;
		XString m_SkyBoxFile;
	};
}