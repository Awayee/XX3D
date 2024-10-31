#include "Objects/Public/StaticMesh.h"
#include "Objects/Public/RenderScene.h"
#include "Asset/Public/AssetLoader.h"
#include "Objects/Public/MeshRenderer.h"

namespace Object {

	void TransformComponent::OnLoad(const Json::Value& val) {
		Json::LoadFloatArray(val["Position"], Transform.Position.Data(), 3);
		Json::LoadFloatArray(val["Scale"], Transform.Scale.Data(), 3);
		Math::FVector3 euler;
		Json::LoadFloatArray(val["Rotation"], euler.Data(), 3);
		Transform.Rotation = Math::FQuaternion::Euler(euler);
		TransformUpdated();
	}

	void TransformComponent::OnAdd() {
		GetScene()->AddComponent<TransformECSComponent>(GetEntityID());
	}

	void TransformComponent::OnRemove() {
		GetScene()->RemoveComponent<TransformECSComponent>(GetEntityID());
	}

	void TransformComponent::TransformUpdated() {
		TransformECSComponent* component = GetScene()->GetComponent<TransformECSComponent>(GetEntityID());
		component->SetTransform(Transform);
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
		GetScene()->AddComponent<MeshECSComponent>(GetEntityID());
	}

	void MeshComponent::OnRemove() {
		GetScene()->RemoveComponent<MeshECSComponent>(GetEntityID());
	}

	void MeshComponent::SetMeshFile(const XString& meshFile) {
		m_MeshFile = meshFile;
		Asset::MeshAsset asset;
		if(Asset::AssetLoader::LoadProjectAsset(&asset, m_MeshFile.c_str())) {
			MaterialContainer& materialContainer = GetScene()->GetMaterialContainer();
			MeshECSComponent* component = GetScene()->GetComponent<MeshECSComponent>(GetEntityID());
			auto& Primitives = component->Primitives;
			Primitives.Reset();
			Primitives.Reserve(asset.Primitives.Size());
			for (auto& srcPrimitive : asset.Primitives) {
				auto& primitive = Primitives.EmplaceBack();
				primitive.Primitive = Object::StaticResourceMgr::Instance()->GetPrimitive(srcPrimitive.BinaryFile);
				primitive.MaterialIndex = materialContainer.FindOrCreateMaterial(srcPrimitive.MaterialFile);
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
		}
	}

	void MeshComponent::SetCastShadow(bool bCastShadow) {
		m_CastShadow = bCastShadow;
		MeshECSComponent* component = GetScene()->GetComponent<MeshECSComponent>(GetEntityID());
		component->CastShadow = bCastShadow;
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
