#include "WndLevelSetting.h"
#include "Functions/Public/EditorLevel.h"
#include "Objects/Public/DirectionalLight.h"
#include "Math/Public/Math.h"
#include "Objects/Public/Camera.h"
#include "EditorUI/Public/EditorUIMgr.h"

namespace Editor {

	WndLevelSetting::WndLevelSetting(): EditorWndBase("LevelSetting") {
		EditorUIMgr::Instance()->AddMenu("Window", m_Name, {}, &m_Enable);
	}

	WndLevelSetting::~WndLevelSetting() {
	}

	void WndLevelSetting::WndContent() {
		EditorLevel* level = Editor::EditorLevelMgr::Instance()->GetLevel();
		if (!level) {
			return;
		}
		Object::RenderScene* scene = level->GetScene();
		if (!scene) {
			return;
		}
		if(ImGui::CollapsingHeader("DirectionalLight", ImGuiTreeNodeFlags_DefaultOpen)) {
			Object::DirectionalLight* light = scene->GetDirectionalLight();
			Math::FVector3 lightRotation = light->GetRotation();
			if(ImGui::DragFloat3("Rotation", lightRotation.Data(), 0.01f, -4.0f, 4.0f)) {
				light->SetRotation(lightRotation);
			}
			Math::FVector3 lightColor = light->GetColor();
			if(ImGui::ColorPicker3("Color", lightColor.Data(), ImGuiColorEditFlags_None)) {
				light->SetColor(lightColor);
			}
			Object::DirectionalShadowConfig config = light->GetShadowConfig();
			bool isDirty{ false };
			isDirty |= ImGui::Checkbox("Enable Shadow", &config.EnableShadow);
			isDirty |= ImGui::Checkbox("Shadow Debug", &config.EnableDebug);
			isDirty |= ImGui::DragFloat("Shadow Distance", &config.ShadowDistance, 1.0f, 0.0f, 9999.0f);
			isDirty |= ImGui::DragFloat("Log Distribution", &config.LogDistribution, 0.01f, 0.0f, 1.0f);
			static TStaticArray<uint32, 5> s_ShadowMapSizes{ 256, 512, 1024, 2048, 4096 };
			static TStaticArray<const char*, 5> s_ShadowMapSizeNames{ "256", "512", "1024", "2048", "4096" };
			const uint32 shadowMapSize = config.ShadowMapSize;
			int currentItem;
			for(int i=0; i<(int)s_ShadowMapSizes.Size(); ++i) {
				if (s_ShadowMapSizes[i] == shadowMapSize) {
					currentItem = i;
					break;
				}
			}
			if(ImGui::Combo("ShadowMap Size", &currentItem, s_ShadowMapSizeNames.Data(), s_ShadowMapSizeNames.Size())) {
				config.ShadowMapSize = s_ShadowMapSizes[currentItem];
				isDirty = true;
			}
			isDirty |= ImGui::DragFloat("Bias Const", &config.ShadowBiasConstant, 0.01f, 0.0f, 100.0f);
			isDirty |= ImGui::DragFloat("Bias Slope", &config.ShadowBiasSlope, 0.01f, 0.0f, 100.0f);
			if(isDirty) {
				light->SetShadowConfig(config);
			}
		}
		if(ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
			Object::RenderCamera* camera = scene->GetMainCamera();
			{   // view
				auto cameraView = camera->GetView();
				bool modified = ImGui::DragFloat3("Eye", cameraView.Eye.Data());
				modified |= ImGui::DragFloat3("At", cameraView.At.Data());
				modified |= ImGui::DragFloat3("Up", cameraView.Up.Data());
				if(modified) {
					camera->SetView({cameraView.Eye, cameraView.At, cameraView.Up});
				}
			}
			{   // projection
				auto projection = camera->GetProjection();
				bool modified = ImGui::Combo("ProjType", (int*)&projection.ProjType, "Perspective\0Ortho");
				modified |= ImGui::DragFloat("Near", &projection.Near, 0.1f, 0.0f, 9999.0f);
				modified |= ImGui::DragFloat("Far", &projection.Far, 0.1f, 1.0f, 9999.0f);
				if(projection.ProjType == Object::EProjType::Perspective) {
					modified |= ImGui::DragFloat("Fov", &projection.Fov, 0.1f, 0.1f, 3.1f); // almost from 0 to PI
				}
				else if (projection.ProjType == Object::EProjType::Ortho) {
					modified |= ImGui::DragFloat("Size", &projection.ViewSize, 0.1f, 0.1f, 9999.0f);
				}
				if(modified) {
					camera->SetProjection(projection);
				}
			}
		}
	}
}
