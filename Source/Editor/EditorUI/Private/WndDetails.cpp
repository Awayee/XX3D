#include "WndDetails.h"
#include "AssetView.h"
#include "Functions/Public/EditorLevelMgr.h"

namespace Editor {

	void WndDetails::Update() {
	}

	void WndDetails::Display() {
		EditorLevelMgr* inst = EditorLevelMgr::Instance();
		EditorLevel* level = inst->GetLevel();
		if(!level) {
			return;
		}
		uint32 idx = inst->GetSelected();
		if(idx == UINT32_MAX) {
			return;
		}
		auto mesh = level->GetMesh(idx);
		if(ImGui::DragFloat3("Position", mesh->Position.Data(), 0.01f)) {
			mesh->Mesh->SetPosition(mesh->Position);
		}
		if (ImGui::DragFloat3("Scale", mesh->Scale.Data(), 0.01f)) {
			mesh->Mesh->SetScale(mesh->Scale);
		}
		if(ImGui::DragFloat3("Rotation", mesh->Rotation.Data(), 0.01f)) {
			mesh->Mesh->SetRotation(Math::FQuaternion::Euler(mesh->Rotation));
		}
	}

	WndDetails::WndDetails(): EditorWindowBase("Details") {
	}
}
