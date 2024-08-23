#include "ClientCode/Public/RuntimeLevel.h"
#include "Asset/Public/AssetLoader.h"

namespace Runtime {
	RuntimeLevel::RuntimeLevel(const Asset::LevelAsset& asset, Object::RenderScene* scene): Level(asset, scene) {
		//meshes
		auto& objects = asset.Meshes;
		for (auto& mesh : objects) {
			Asset::MeshAsset meshAsset;
			if (Asset::AssetLoader::LoadProjectAsset(&meshAsset, mesh.File.c_str())) {
				Object::EntityID meshID = scene->NewEntity();
				auto* transformCom = scene->AddComponent<Object::TransformComponent>(meshID);
				transformCom->SetPosition(mesh.Position);
				transformCom->SetScale(mesh.Scale);
				transformCom->SetRotation(Math::FQuaternion::Euler(mesh.Rotation));
				auto* staticMeshCom = scene->AddComponent<Object::StaticMeshComponent>(meshID);
				staticMeshCom->BuildFromAsset(meshAsset);
			}
		}
	}
}
