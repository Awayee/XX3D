#include "Functions/Public/EditorLevel.h"
#include "Objects/Public/Camera.h"
#include "Functions/Public/AssetManager.h"

namespace Editor {

	EditorLevel::EditorLevel(const Engine::ALevelAsset& asset, Engine::RenderScene* scene): m_Scene(scene) {
		//camera
		auto& cameraParam = asset.CameraParam;
		Engine::Camera* camera = scene->GetMainCamera();
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
				EditorLevelMesh levelMesh;
				levelMesh.Asset = node->GetAsset<Engine::AMeshAsset>();
				levelMesh.Position = meshData.Position;
				levelMesh.Scale = meshData.Scale;
				levelMesh.Rotation = meshData.Rotation;
				levelMesh.Name = meshData.Name;
				levelMesh.File = meshData.File;
				levelMesh.Mesh.Reset(new Engine::StaticMesh(*levelMesh.Asset, m_Scene));
				levelMesh.Mesh->SetPosition(levelMesh.Position);
				levelMesh.Mesh->SetScale(levelMesh.Scale);
				levelMesh.Mesh->SetRotation(Math::FQuaternion::Euler(levelMesh.Rotation));
				m_Meshes.PushBack(levelMesh);
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

	EditorLevelMesh* EditorLevel::AddMesh(const XXString& file, Engine::AMeshAsset* asset) {
		EditorLevelMesh mesh{};
		mesh.File = file;
		mesh.Name = asset->Name;
		mesh.Asset = asset;
		mesh.Mesh.Reset(new Engine::StaticMesh(*asset, m_Scene));
		mesh.Position = mesh.Mesh->GetPosition();
		mesh.Scale = mesh.Mesh->GetScale();
		mesh.Rotation = mesh.Mesh->GetRotation().ToEuler();
		m_Meshes.PushBack(mesh);
		return &m_Meshes.Back();
	}

	void EditorLevel::DelMesh(uint32 idx) {
		m_Meshes.RemoveAt(idx);
	}

	void EditorLevel::SaveAsset(Engine::ALevelAsset* asset) {
		Engine::Camera* camera = m_Scene->GetMainCamera();
		asset->CameraParam.Eye = camera->GetView().Eye;
		asset->CameraParam.At = camera->GetView().At;
		asset->CameraParam.Up = camera->GetView().Up;
		asset->CameraParam.Near = camera->GetNear();
		asset->CameraParam.Far = camera->GetFar();
		asset->CameraParam.Fov = camera->GetFov();

		asset->Meshes.Clear();
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
