#include "WndDetails.h"
#include "AssetView.h"
#include "EditorUI/Public/EditorUIMgr.h"
#include "Functions/Public/EditorLevelMgr.h"

namespace Editor {

	void WndDetails::Update() {
	}

	void WndDetails::WndContent() {
		EditorLevelMgr* inst = EditorLevelMgr::Instance();
		EditorLevel* level = inst->GetLevel();
		if(!level) {
			return;
		}
		uint32 idx = inst->GetSelected();
		if(idx == UINT32_MAX) {
			return;
		}
		EditorLevelMesh* mesh = level->GetMesh(idx);

		//Transform
		if(ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (ImGui::DragFloat3("Position", mesh->Position.Data(), 0.1f)) {
				level->GetScene()->GetComponent<Object::TransformComponent>(mesh->ObjectEntityID)->SetPosition(mesh->Position);
			}
			if (ImGui::DragFloat3("Scale", mesh->Scale.Data(), 0.01f)) {
				level->GetScene()->GetComponent<Object::TransformComponent>(mesh->ObjectEntityID)->SetScale(mesh->Scale);
			}
			if (ImGui::DragFloat3("Rotation", mesh->Rotation.Data(), 0.01f)) {
				level->GetScene()->GetComponent<Object::TransformComponent>(mesh->ObjectEntityID)->SetRotation(Math::FQuaternion::Euler(mesh->Rotation));
			}
		}

		//Material
		if(ImGui::CollapsingHeader("Materials", ImGuiTreeNodeFlags_DefaultOpen)) {
			for (auto& primitive : mesh->Asset->Primitives) {
				//material
				File::FPath primitivePath = primitive.BinaryFile;
				ImGui::Text(primitivePath.stem().string().c_str()); ImGui::SameLine();
				if (ImGui::BeginDragDropTarget()) {
					ImGui::Button(primitive.MaterialFile.empty() ? "None" : primitive.MaterialFile.c_str());
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("File")) {
						ASSERT(payload->DataSize == sizeof(FileNode), "");
						const FileNode* fileNode = reinterpret_cast<const FileNode*>(payload->Data);
						if(fileNode->GetExt() == ".texture") {
							primitive.MaterialFile = fileNode->GetPathStr();
							EditorLevelMgr::Instance()->ReloadLevel();
						}
					}
					ImGui::EndDragDropTarget();
				}
				else {
					ImGui::Button(primitive.MaterialFile.empty() ? "None" : primitive.MaterialFile.c_str());
				}
			}
		}
	}

	WndDetails::WndDetails(): EditorWndBase("Details") {
		EditorUIMgr::Instance()->AddMenu("Window", m_Name, {}, &m_Enable);
	}
}
