#include "Functions/Public/EditorLevel.h"
#include "Objects/Public/Camera.h"
#include "Functions/Public/AssetManager.h"

namespace Editor {

	EditorLevel::EditorLevel(const Asset::LevelAsset& asset, Object::RenderScene* scene): m_Scene(scene) {
		//camera
		auto& cameraParam = asset.CameraParam;
		Object::Camera* camera = scene->GetMainCamera();
		camera->SetView(cameraParam.Eye, cameraParam.At, cameraParam.Up);
		camera->SetNear(cameraParam.Near);
		camera->SetFar(cameraParam.Far);
		camera->SetFov(cameraParam.Fov);

		//meshes
		auto& meshes = asset.Meshes;
		m_Meshes.Reserve(meshes.Size());
		for (auto& meshData : meshes) {
			FileNode* node = ProjectAssetMgr::Instance()->GetFile(meshData.File);
			if (node) {
				EditorLevelMesh& levelMesh = m_Meshes.EmplaceBack();
				levelMesh.Asset = node->GetAsset<Asset::MeshAsset>();
				levelMesh.Position = meshData.Position;
				levelMesh.Scale = meshData.Scale;
				levelMesh.Rotation = meshData.Rotation;
				levelMesh.Name = meshData.Name;
				levelMesh.File = meshData.File;
				levelMesh.Mesh.Reset(new Object::StaticMesh(*levelMesh.Asset, m_Scene));
				levelMesh.Mesh->SetPosition(levelMesh.Position);
				levelMesh.Mesh->SetScale(levelMesh.Scale);
				levelMesh.Mesh->SetRotation(Math::FQuaternion::Euler(levelMesh.Rotation));
			}
		}
	}

	EditorLevel::~EditorLevel() {
		
	}

	TVector<EditorLevelMesh>& EditorLevel::Meshes() {
		return m_Meshes;
	}

	EditorLevelMesh* EditorLevel::GetMesh(uint32 idx) {
		return &m_Meshes[idx];
	}

	EditorLevelMesh* EditorLevel::AddMesh(const XString& file, Asset::MeshAsset* asset) {
		EditorLevelMesh& mesh = m_Meshes.EmplaceBack();
		mesh.File = file;
		mesh.Name = asset->Name;
		mesh.Asset = asset;
		mesh.Mesh.Reset(new Object::StaticMesh(*asset, m_Scene));
		mesh.Position = mesh.Mesh->GetPosition();
		mesh.Scale = mesh.Mesh->GetScale();
		mesh.Rotation = mesh.Mesh->GetRotation().ToEuler();
		return &mesh;
	}

	void EditorLevel::DelMesh(uint32 idx) {
		m_Meshes.RemoveAt(idx);
	}

	void EditorLevel::SaveAsset(Asset::LevelAsset* asset) {
		Object::Camera* camera = m_Scene->GetMainCamera();
		asset->CameraParam.Eye = camera->GetView().Eye;
		asset->CameraParam.At = camera->GetView().At;
		asset->CameraParam.Up = camera->GetView().Up;
		asset->CameraParam.Near = camera->GetNear();
		asset->CameraParam.Far = camera->GetFar();
		asset->CameraParam.Fov = camera->GetFov();

		asset->Meshes.Reset();
		asset->Meshes.Resize(m_Meshes.Size());
		for(uint32 i=0; i<m_Meshes.Size(); ++i) {
			auto& savedMesh = asset->Meshes[i];
			auto& mesh = m_Meshes[i];
			savedMesh.File = mesh.File;
			savedMesh.Name = mesh.Name;
			savedMesh.Position = mesh.Position;
			savedMesh.Scale = mesh.Scale;
			savedMesh.Rotation = mesh.Rotation;
		}
	}
}
