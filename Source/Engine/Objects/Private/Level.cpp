#include "Objects/Public/Level.h"
#include "Objects/Public/Camera.h"
#include "Asset/Public/AssetLoader.h"
#include "Objects/Public/StaticMesh.h"

namespace Object {

	Level::Level(const Asset::LevelAsset& levelAsset, RenderScene* scene): m_Scene(scene) {
		//meshes
		auto& objects = levelAsset.Meshes;
		for(auto& mesh: objects) {
			Asset::MeshAsset meshAsset;
			if(Asset::AssetLoader::LoadProjectAsset(&meshAsset, mesh.File.c_str())) {
				EntityID meshID = scene->NewEntity();
				auto* transformCom = scene->AddComponent<TransformComponent>(meshID);
				transformCom->SetPosition(mesh.Position);
				transformCom->SetScale(mesh.Scale);
				transformCom->SetRotation(Math::FQuaternion::Euler(mesh.Rotation));
				auto* staticMeshCom = scene->AddComponent<StaticMeshComponent>(meshID);
				staticMeshCom->BuildFromAsset(meshAsset);
			}
		}
		//camera
		auto& cameraParam = levelAsset.CameraParam;
		Camera* camera = scene->GetMainCamera();
		camera->SetView(cameraParam.Eye, cameraParam.At, cameraParam.Up);
		camera->SetNear(cameraParam.Near);
		camera->SetFar(cameraParam.Far);
		camera->SetFov(cameraParam.Fov);
	}

	Level::~Level() {
	}
}
