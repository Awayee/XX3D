#include "Objects/Public/LevelComponents.h"
#include "Objects/Public/DirectionalLight.h"
#include "Objects/Public/Camera.h"
#include "System/Public/Configuration.h"

namespace Object {

	void CameraComponent::OnLoad(const Json::Value& val) {
		Json::LoadFloatArray(val["Eye"], Eye.Data(), 3);
		Json::LoadFloatArray(val["At"], At.Data(), 3);
		Json::LoadFloatArray(val["Up"], Up.Data(), 3);
		ProjType = (Object::EProjType)val["ProjType"].GetInt();
		Near = val["Near"].GetFloat();
		Far = val["Far"].GetFloat();
		Fov = val["Fov"].GetFloat();
		HalfHeight = val["HalfHeight"].GetFloat();
		SyncData();
	}

	void CameraComponent::OnAdd() {
		// init as default values
		Eye = { 0, 4, -4 };
		At = { 0, 2, 0 };
		Up = { 0, 1, 0 };
		ProjType = EProjType::Perspective;
		Near = 0.2f;
		Far = 1000.0f;
		Fov = 0.8f;
		HalfHeight = 2.4f;
		SyncData();
	}

	void CameraComponent::SyncData(DirtyFlags flags) {
		if(Object::RenderScene* scene = GetScene()) {
			Object::RenderCamera* camera = scene->GetMainCamera();
			if(flags.View) {
				camera->SetView({ Eye, At, Up });
			}
			if(flags.Proj) {
				float aspect = camera->GetProjection().Aspect;
				if (EProjType::Ortho == ProjType) {
					const float halfHeight = HalfHeight;
					const float halfWidth = halfHeight * aspect;
					camera->SetProjection(CameraProjection::Orthographic(-halfWidth, halfWidth, -halfHeight, halfHeight, Near, Far));
				}
				else if (EProjType::Perspective == ProjType) {
					camera->SetProjection(CameraProjection::Perspective(Fov, aspect, Near, Far));
				}
			}
		}
	}

	void DirectionalLightComponent::OnLoad(const Json::Value& val) {
		Json::LoadFloatArray(val["Rotation"], Rotation.Data(), 3);
		Json::LoadFloatArray(val["Color"], Color.Data(), 4);
		if(val.HasMember("EnableShadow")) {
			EnableShadow = val["EnableShadow"].GetBool();
		}
		if(val.HasMember("ShadowDistance")) {
			ShadowDistance = val["ShadowDistance"].GetFloat();
		}
		if(val.HasMember("ShadowLogDistribution")) {
			ShadowLogDistribution = val["ShadowLogDistribution"].GetFloat();
		}
		if (val.HasMember("ShadowMapSize")) {
			ShadowMapSize = val["ShadowMapSize"].GetUint();
		}
		if (val.HasMember("ShadowBiasConst")) {
			ShadowBiasConst = val["ShadowBiasConst"].GetFloat();
		}
		if (val.HasMember("ShadowBiasSlope")) {
			ShadowBiasSlope = val["ShadowBiasSlope"].GetFloat();
		}
		SyncData();
	}

	void DirectionalLightComponent::OnAdd() {
		// init as default
		Rotation = { 0.580f, 3.710f, 0.0f };
		Color = { 1.0f, 1.0f, 1.0f, 3.0f };
		EnableShadow = true;
		ShadowDebug = false;
		ShadowMapSize = Engine::ProjectConfig::Instance().DefaultShadowMapSize;
		ShadowDistance = 128.0f;
		ShadowLogDistribution = 0.9f;
		ShadowBiasConst = 3.0f;
		ShadowBiasSlope = 3.0f;
		SyncData();
	}

	void DirectionalLightComponent::SyncData(DirtyFlags flags) {
		if (Object::RenderScene* scene = GetScene()) {
			Object::DirectionalLight* directionalLight = scene->GetDirectionalLight();
			if(flags.Light) {
				directionalLight->SetRotation(Rotation);
				directionalLight->SetColor(Color);
			}
			if(flags.Shadow) {
				const DirectionalShadowConfig config{ EnableShadow, ShadowDebug, ShadowMapSize, ShadowDistance, ShadowLogDistribution, ShadowBiasConst, ShadowBiasSlope };
				directionalLight->SetShadowConfig(config);
			}
		}
	}
}
