#include "WndDetails.h"
#include "AssetView.h"
#include "UIExtent.h"
#include "EditorUI/Public/EditorUIMgr.h"
#include "Functions/Public/EditorLevel.h"
#include "Objects/Public/SkyBox.h"
#include "Objects/Public/StaticMesh.h"
#include "Objects/Public/Level.h"

namespace Editor {

	void TransformGUI(Object::TransformComponent* com) {
		auto& transform = com->Transform;
		bool dirty = ImGui::DragFloat3("Position", transform.Position.Data(), 0.1f);
		dirty |= ImGui::DragFloat3("Scale", transform.Scale.Data(), 0.01f);
		dirty |= ImGui::DragFloat3("Rotation", transform.Rotation.Data(), 0.01f);
		if (dirty) {
			com->TransformUpdated();
		}
	}
	REGISTER_LEVEL_EDIT_OnGUI(Object::TransformComponent, TransformGUI);

	void MeshGUI(Object::MeshComponent* com) {
		XString file = com->GetMeshFile();
		if (ImGui::DraggableFileItemAssets("File##Mesh", file, "*.mesh")) {
			com->SetMeshFile(file);
		}
	}
	REGISTER_LEVEL_EDIT_OnGUI(Object::MeshComponent, MeshGUI);

	void InstancedDataGUI(Object::InstanceDataComponent* com) {
		XString file = com->GetInstanceFile();
		if (ImGui::DraggableFileItemAssets("File##InstanceData", file, "*.instd")) {
			com->SetInstanceFile(file);
		}
	}
	REGISTER_LEVEL_EDIT_OnGUI(Object::InstanceDataComponent, InstancedDataGUI);

	void SkyBoxGUI(Object::SkyBoxComponent* com) {
		XString skyBoxFile = com->GetCubeMapFile();
		if (ImGui::DraggableFileItemAssets("File##SkyBox", skyBoxFile, "*.texture")) {
			com->SetCubeMapFile(skyBoxFile);
		}
	}
	REGISTER_LEVEL_EDIT_OnGUI(Object::SkyBoxComponent, SkyBoxGUI);

	WndDetails::WndDetails() : EditorWndBase("Details") {
		EditorUIMgr::Instance()->AddMenu("Window", m_Name, {}, &m_Enable);
	}

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
		Object::LevelActor* actor = level->GetActor(idx);
		TArrayView<Object::LevelComponent*> components = actor->GetComponents();
		uint32 mouseOnIdx = INVALID_INDEX;
		for (uint32 i = 0; i<components.Size(); ++i) {
			Object::LevelComponent* com = components[i];
			if(ImGui::CollapsingHeader(Object::LevelComponentFactory::Instance().GetTypeInfo(com)->GetTypeName().c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
				if (ImGui::IsItemHovered()) {
					mouseOnIdx = i;
				}
				LevelComponentEditProxyFactory::Instance().GetProxy(com).OnGUI(com);
			}
		}
		if(ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
			m_EditCompIdx = mouseOnIdx;
			m_IsAddingComponent = false;
		}

		if(ImGui::BeginPopupContextWindow("Details_ContextMenu", ImGuiPopupFlags_MouseButtonRight)) {
			if(m_EditCompIdx != INVALID_INDEX) {
				if(ImGui::MenuItem("Remove")) {
					actor->RemoveComponent(components[m_EditCompIdx]);
					m_EditCompIdx = INVALID_INDEX;
				}
			}
			else if(ImGui::MenuItem("Add")) {
				m_IsAddingComponent = true;
			}
			ImGui::EndPopup();
		}
		else {
			m_EditCompIdx = INVALID_INDEX;
		}
		if (m_IsAddingComponent) {
			ImGui::OpenPopup("Details_AddComponent");
			if(ImGui::BeginPopup("Details_AddComponent")) {
				if (ImGui::ImGuiIsMouseClicked() && !ImGui::IsWindowHovered()) {
					m_IsAddingComponent = false;
				}
				auto& fac = Object::LevelComponentFactory::Instance();
				for (uint32 i = 0; i < fac.GetTypeSize(); ++i) {
					auto* typeInfo = fac.GetTypeInfo(i);
					if (ImGui::MenuItem(typeInfo->GetTypeName().c_str())) {
						typeInfo->AddComponent(actor);
						m_IsAddingComponent = false;
					}
				}
				ImGui::EndPopup();
			}
		}
	}
}
