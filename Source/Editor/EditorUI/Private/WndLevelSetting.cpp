#include "WndLevelSetting.h"
#include "Functions/Public/EditorLevelMgr.h"
#include "Objects/Public/DirectionalLight.h"
#include "Math/Public/Math.h"

namespace Editor {

	WndLevelSetting::WndLevelSetting(): EditorWndBase("LevelSetting") {
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
			Math::FVector3 lightDir = light->GetLightDir();
			ImGui::Text("Direction"); ImGui::SameLine();
			if(ImGui::DragFloat3("##Direction", lightDir.Data())) {
				light->SetDir(lightDir);
			}
			Math::FVector3 lightColor = light->GetLightColor();
			ImGui::Text("Color"); ImGui::SameLine();
			if(ImGui::ColorPicker3("##Color", lightColor.Data())) {
				light->SetColor(lightColor);
			}
		}
	}
}
