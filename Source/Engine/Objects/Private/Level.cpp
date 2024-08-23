#include "Objects/Public/Level.h"
#include "Objects/Public/Camera.h"
#include "Asset/Public/AssetLoader.h"
#include "Objects/Public/StaticMesh.h"
#include "Objects/Public/DirectionalLight.h"

namespace Object {

	Level::Level(const Asset::LevelAsset& asset, RenderScene* scene): m_Scene(scene) {

		//camera
		auto& cameraParam = asset.CameraData;
		Object::Camera* camera = scene->GetMainCamera();
		camera->SetView(cameraParam.Eye, cameraParam.At, cameraParam.Up);
		camera->SetNear(cameraParam.Near);
		camera->SetFar(cameraParam.Far);
		camera->SetFov(cameraParam.Fov);
		// light
		auto& lightParam = asset.DirectionalLightData;
		Object::DirectionalLight* directionalLight = scene->GetDirectionalLight();
		directionalLight->SetDir(lightParam.Dir);
		directionalLight->SetColor(lightParam.Color);
	}

	Level::~Level() {
	}
}
