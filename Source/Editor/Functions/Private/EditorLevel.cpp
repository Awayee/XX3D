#include "Functions/Public/EditorLevel.h"
#include "Objects/Public/Camera.h"
#include "Functions/Public/AssetManager.h"
#include "Objects/Public/DirectionalLight.h"

namespace Editor {

	EditorLevel::EditorLevel(const Asset::LevelAsset& asset, Object::RenderScene* scene): Object::Level(asset, scene) {
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
				levelMesh.ObjectEntityID = scene->NewEntity();
				auto* transform = scene->AddComponent<Object::TransformComponent>(levelMesh.ObjectEntityID);
				transform->SetPosition(levelMesh.Position);
				transform->SetScale(levelMesh.Scale);
				transform->SetRotation(Math::FQuaternion::Euler(levelMesh.Rotation));
				auto* staticMesh = scene->AddComponent<Object::StaticMeshComponent>(levelMesh.ObjectEntityID);
				staticMesh->BuildFromAsset(*levelMesh.Asset);
			}
		}
	}

	EditorLevel::~EditorLevel() {
		
	}

	Object::RenderScene* EditorLevel::GetScene() {
		return m_Scene;
	}

	TArray<EditorLevelMesh>& EditorLevel::Meshes() {
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
		mesh.ObjectEntityID = m_Scene->NewEntity();
		auto* transform = m_Scene->AddComponent<Object::TransformComponent>(mesh.ObjectEntityID);
		auto* staticMesh = m_Scene->AddComponent<Object::StaticMeshComponent>(mesh.ObjectEntityID);
		staticMesh->BuildFromAsset(*asset);
		mesh.Position = transform->GetPosition();
		mesh.Scale = transform->GetScale();
		mesh.Rotation = transform->GetRotation().ToEuler();
		return &mesh;
	}

	void EditorLevel::DelMesh(uint32 idx) {
		m_Scene->RemoveEntity(m_Meshes[idx].ObjectEntityID);
		m_Meshes.RemoveAt(idx);
	}

	void EditorLevel::SaveAsset(Asset::LevelAsset* asset) {
		Object::Camera* camera = m_Scene->GetMainCamera();
		asset->CameraData.Eye = camera->GetView().Eye;
		asset->CameraData.At = camera->GetView().At;
		asset->CameraData.Up = camera->GetView().Up;
		asset->CameraData.Near = camera->GetNear();
		asset->CameraData.Far = camera->GetFar();
		asset->CameraData.Fov = camera->GetFov();

		Object::DirectionalLight* dLight = m_Scene->GetDirectionalLight();
		asset->DirectionalLightData.Dir = dLight->GetLightDir();
		asset->DirectionalLightData.Color = dLight->GetLightColor();

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
