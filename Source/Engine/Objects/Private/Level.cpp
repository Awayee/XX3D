#include "Objects/Public/Level.h"
#include "Objects/Public/Camera.h"
#include "Asset/Public/AssetLoader.h"
namespace Engine {

	Level::Level(const ALevelAsset& levelAsset, RenderScene* scene): m_Scene(scene) {
		//meshes
		auto& objects = levelAsset.Meshes;
		for(auto& mesh: objects) {
			AMeshAsset meshAsset;
			if(AssetLoader::LoadProjectAsset(&meshAsset, mesh.File.c_str())) {
				TUniquePtr<StaticMesh> staticMesh(new StaticMesh(meshAsset, m_Scene));
				staticMesh->SetPosition(mesh.Position);
				staticMesh->SetScale(mesh.Scale);
				staticMesh->SetRotation(Math::FQuaternion::Euler(mesh.Rotation));
				m_Meshes.PushBack(std::move(staticMesh));
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
