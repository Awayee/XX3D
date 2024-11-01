#include "WndLevelSetting.h"
#include "Functions/Public/EditorLevel.h"
#include "Math/Public/Math.h"
#include "EditorUI/Public/EditorUIMgr.h"
#include "Objects/Public/LevelComponents.h"

namespace Editor {

	void CameraGUI(Object::CameraComponent* com) {
		Object::CameraComponent::DirtyFlags dirty = 0;
		// view
		dirty.View |= ImGui::DragFloat3("Eye", com->Eye.Data(), 0.1f);
		dirty.View |= ImGui::DragFloat3("At", com->At.Data(), 0.01f);
		dirty.View |= ImGui::DragFloat3("Up", com->Up.Data(), 0.01f);
		// projection
		dirty.Proj |= ImGui::Combo("ProjType", (int*)&com->ProjType, "Perspective\0Ortho");
		dirty.Proj |= ImGui::DragFloat("Near", &com->Near, 0.1f, 0.0f, 9999.0f);
		dirty.Proj |= ImGui::DragFloat("Far", &com->Far, 0.1f, 1.0f, 9999.0f);
		if (com->ProjType == Object::EProjType::Perspective) {
			dirty.Proj |= ImGui::DragFloat("Fov", &com->Fov, 0.1f, 0.1f, 3.1f); // almost from 0 to PI
		}
		else if (com->ProjType == Object::EProjType::Ortho) {
			dirty.Proj |= ImGui::DragFloat("Size", &com->HalfHeight, 0.1f, 0.1f, 9999.0f);
		}
		if(dirty.Value) {
			com->SyncData(dirty);
		}
	}
	REGISTER_LEVEL_EDIT_OnGUI(Object::CameraComponent, CameraGUI);

	void DirectionalLightGUI(Object::DirectionalLightComponent* com) {
		Object::DirectionalLightComponent::DirtyFlags dirty = 0;
		// light
		dirty.Light |= ImGui::DragFloat3("Rotation", com->Rotation.Data(), 0.01f, -Math::PI * 2.0f, Math::PI * 2.0f);
		dirty.Light |= ImGui::ColorEdit3("Color", com->Color.Data(), ImGuiColorEditFlags_None);
		dirty.Light |= ImGui::DragFloat("Intensity", &com->Color.W, 0.01f, 0.0f, 9999.0f);
		// shadow
		dirty.Shadow |= ImGui::Checkbox("Enable Shadow", &com->EnableShadow);
		dirty.Shadow |= ImGui::Checkbox("Shadow Debug", &com->ShadowDebug);
		dirty.Shadow |= ImGui::DragFloat("Shadow Distance", &com->ShadowDistance, 1.0f, 0.0f, 9999.0f);
		dirty.Shadow |= ImGui::DragFloat("Log Distribution", &com->ShadowLogDistribution, 0.01f, 0.0f, 1.0f);
		static TStaticArray<uint32, 5> s_ShadowMapSizes{ 256, 512, 1024, 2048, 4096 };
		static TStaticArray<const char*, 5> s_ShadowMapSizeNames{ "256", "512", "1024", "2048", "4096" };
		const uint32 shadowMapSize = com->ShadowMapSize;
		int currentItem;
		for (int i = 0; i < (int)s_ShadowMapSizes.Size(); ++i) {
			if (s_ShadowMapSizes[i] == shadowMapSize) {
				currentItem = i;
				break;
			}
		}
		if (ImGui::Combo("ShadowMap Size", &currentItem, s_ShadowMapSizeNames.Data(), s_ShadowMapSizeNames.Size())) {
			com->ShadowMapSize = s_ShadowMapSizes[currentItem];
			dirty.Shadow = true;
		}
		dirty.Shadow |= ImGui::DragFloat("Bias Const", &com->ShadowBiasConst, 0.01f, 0.0f, 100.0f);
		dirty.Shadow |= ImGui::DragFloat("Bias Slope", &com->ShadowBiasSlope, 0.01f, 0.0f, 100.0f);

		if(dirty.Value) {
			com->SyncData(dirty);
		}
	}
	REGISTER_LEVEL_EDIT_OnGUI(Object::DirectionalLightComponent, DirectionalLightGUI);


	WndLevelSetting::WndLevelSetting(): EditorWndBase("LevelSetting") {
		EditorUIMgr::Instance()->AddMenu("Window", m_Name.c_str(), {}, &m_Enable);
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
		TArrayView<Object::LevelComponentBase*> components = level->GetComponents();
		for(Object::LevelComponentBase* com: components) {
			if (ImGui::CollapsingHeader(Object::LevelComponentFactory::Instance().GetTypeInfo(com)->GetTypeName().c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
				LevelComponentEditProxyFactory::Instance().GetProxy(com).OnGUI(com);
			}
		}
	}
}
