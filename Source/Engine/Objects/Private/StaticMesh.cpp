#include "Objects/Public/StaticMesh.h"
#include "Objects/Public/RenderScene.h"
#include "Asset/Public/AssetLoader.h"
#include "Objects/Public/Material.h"
#include "Objects/Public/MeshRenderer.h"

namespace Object {

	void TransformComponent::OnLoad(const Json::Value& val) {
		Json::LoadFloatArray(val["Position"], Position.Data(), 3);
		Json::LoadFloatArray(val["Scale"], Scale.Data(), 3);
		Json::LoadFloatArray(val["Rotation"], Euler.Data(), 3);
		TransformUpdated();
	}

	void TransformComponent::OnAdd() {
		Position = { 0,0,0 };
		Scale = { 1,1,1 };
		Euler = { 0,0,0 };
		GetScene()->AddComponent<TransformECSComponent>(GetEntityID());
	}

	void TransformComponent::OnRemove() {
		GetScene()->RemoveComponent<TransformECSComponent>(GetEntityID());
	}

	void TransformComponent::TransformUpdated() {
		Math::FTransform transform;
		transform.Position = Position;
		transform.Rotation = Math::FQuaternion::Euler(Euler);
		transform.Scale = Scale;
		TransformECSComponent* component = GetScene()->GetComponent<TransformECSComponent>(GetEntityID());
		component->SetTransform(transform);
	}

	void MeshComponent::OnLoad(const Json::Value& val) {
		if(val.HasMember("MeshFile")) {
			if(val.HasMember("MeshFile")) {
				SetMeshFile(val["MeshFile"].GetString());
			}

			if(val.HasMember("CastShadow")) {
				SetCastShadow(val["CastShadow"].GetBool());
			}
			else {
				SetCastShadow(true);
			}
		}
	}

	void MeshComponent::OnAdd() {
		MeshECSComponent* ecsCom = GetScene()->AddComponent<MeshECSComponent>(GetEntityID());
		ecsCom->RenderFlags.AddFlag(ERenderPassType::BasePass);
	}

	void MeshComponent::OnRemove() {
		GetScene()->RemoveComponent<MeshECSComponent>(GetEntityID());
	}

	void MeshComponent::SetMeshFile(const XString& meshFile) {
		m_MeshFile = meshFile;
		Asset::MeshAsset asset;
		if(Asset::AssetLoader::LoadProjectAsset(&asset, m_MeshFile.c_str())) {
			MeshECSComponent* component = GetScene()->GetComponent<MeshECSComponent>(GetEntityID());
			auto& Primitives = component->Primitives;
			Primitives.Reset();
			Primitives.Reserve(asset.Primitives.Size());
			for (auto& srcPrimitive : asset.Primitives) {
				auto& primitive = Primitives.EmplaceBack();
				primitive.Primitive = Object::StaticResourceMgr::Instance()->GetPrimitive(srcPrimitive.BinaryFile);
				primitive.Material = Object::MaterialMgr::Instance()->GetMaterialInterface(srcPrimitive.MaterialFile);
			}
			// calc aabb
			if (Primitives.Size()) {
				uint32 i = 0;
				component->AABB = Primitives[i++].Primitive->AABB;
				for (; i < Primitives.Size(); ++i) {
					component->AABB.Union(Primitives[i++].Primitive->AABB);
				}
			}
			else {
				component->AABB = {};
			}
			// set cast shadow
			if(m_CastShadow) {
				component->RenderFlags.AddFlag(ERenderPassType::DirectionalShadow);
			}
		}
	}

	void MeshComponent::SetCastShadow(bool bCastShadow) {
		m_CastShadow = bCastShadow;
		MeshECSComponent* component = GetScene()->GetComponent<MeshECSComponent>(GetEntityID());
		auto& renderFlags = component->RenderFlags;
		if(bCastShadow) {
			renderFlags.AddFlag(ERenderPassType::DirectionalShadow);
		}
		else {
			renderFlags.RemoveFlag(ERenderPassType::DirectionalShadow);
		}
	}

	void InstanceDataComponent::OnLoad(const Json::Value& val) {
		if(val.HasMember("InstanceFile")) {
			SetInstanceFile(val["InstanceFile"].GetString());
		}
	}

	void InstanceDataComponent::OnAdd() {
		GetScene()->AddComponent<InstancedDataECSComponent>(GetEntityID());
	}

	void InstanceDataComponent::OnRemove() {
		GetScene()->RemoveComponent<InstancedDataECSComponent>(GetEntityID());
	}

	void InstanceDataComponent::SetInstanceFile(const XString& file) {
		m_InstanceFile = file;
		const auto* meshCom = GetScene()->GetComponent<MeshECSComponent>(GetEntityID());
		if(!meshCom) {
			LOG_WARNING("[InstanceDataComponent::SetInstanceFile] Mesh not found!");
			return;
		}
		Asset::InstanceDataAsset instanceAsset;
		Asset::AssetLoader::LoadProjectAsset(&instanceAsset, m_InstanceFile.c_str());
		auto* dataCom = GetScene()->GetComponent<InstancedDataECSComponent>(GetEntityID());
		dataCom->InstanceData.Build(meshCom->AABB, instanceAsset.Instances);
	}
}
