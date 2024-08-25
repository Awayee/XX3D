#include "Objects/Public/Level.h"
#include "Objects/Public/Camera.h"
#include "Asset/Public/AssetLoader.h"
#include "Objects/Public/StaticMesh.h"
#include "Objects/Public/DirectionalLight.h"

namespace Object {

	Level::Level(const Asset::LevelAsset& asset, RenderScene* scene): m_Scene(scene) {

		//camera
		{
			auto& param = asset.CameraData;
			Object::RenderCamera* camera = scene->GetMainCamera();
			const Object::CameraView view{ param.Eye, param.At, param.Up };
			camera->SetView(view);
			float aspect = camera->GetProjection().Aspect;
			const Object::CameraProjection projection{ (Object::EProjType)param.ProjType, param.Near, param.Far, aspect, param.Fov, param.ViewSize };
			camera->SetProjection(projection);
		}
		// light
		{
			auto& lightParam = asset.DirectionalLightData;
			Object::DirectionalLight* directionalLight = scene->GetDirectionalLight();
			directionalLight->SetRotation(lightParam.Rotation);
			directionalLight->SetColor(lightParam.Color);
		}
	}

	Level::~Level() {
	}
}
