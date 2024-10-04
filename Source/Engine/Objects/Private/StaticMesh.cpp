#include "Objects/Public/StaticMesh.h"
#include "Objects/Public/DirectionalLight.h"
#include "Objects/Public/RenderScene.h"
#include "Objects/Public/TextureResource.h"
#include "Render/Public/DefaultResource.h"
#include "Asset/Public/AssetLoader.h"

namespace Object {
	void TransformECSComp::SetTransform(const Math::FTransform& transform) {
		m_Transform = transform;
		UpdateMat();
	}

	void TransformECSComp::UpdateMat() {
		m_Transform.BuildMatrix(m_MatrixData.Transform);
		m_Transform.BuildInverseMatrix(m_MatrixData.InverseTransform);
	}

	void TransformComponent::OnLoad(const Json::Value& val) {
		Json::LoadFloatArray(val["Position"], Transform.Position.Data(), 3);
		Json::LoadFloatArray(val["Scale"], Transform.Scale.Data(), 3);
		Math::FVector3 euler;
		Json::LoadFloatArray(val["Rotation"], euler.Data(), 3);
		Transform.Rotation = Math::FQuaternion::Euler(euler);
		TransformUpdated();
	}

	void TransformComponent::OnAdd() {
		GetScene()->AddComponent<TransformECSComp>(GetEntityID());
	}

	void TransformComponent::OnRemove() {
		GetScene()->RemoveComponent<TransformECSComp>(GetEntityID());
	}

	void TransformComponent::TransformUpdated() {
		TransformECSComp* component = GetScene()->GetComponent<TransformECSComp>(GetEntityID());
		component->SetTransform(Transform);
	}

	void MeshECSComp::BuildFromAsset(const Asset::MeshAsset& meshAsset) {
		auto r = RHI::Instance();
		Primitives.Reset();
		Primitives.Reserve(meshAsset.Primitives.Size());
		for(auto& srcPrimitive: meshAsset.Primitives) {
			auto& primitive = Primitives.EmplaceBack();
			primitive.VertexCount = srcPrimitive.Vertices.Size();
			primitive.IndexCount = srcPrimitive.Indices.Size();
			// vb
			RHIBufferDesc vbDesc{EBufferFlags::Vertex | EBufferFlags::CopyDst, srcPrimitive.Vertices.ByteSize(), sizeof(Asset::AssetVertex) };
			primitive.VertexBuffer = r->CreateBuffer(vbDesc);
			primitive.VertexBuffer->UpdateData(srcPrimitive.Vertices.Data(), srcPrimitive.Vertices.ByteSize(), 0);
			// ib
			RHIBufferDesc ibDesc{EBufferFlags::Index | EBufferFlags::CopyDst, srcPrimitive.Indices.ByteSize(), sizeof(Asset::IndexType) };
			primitive.IndexBuffer = r->CreateBuffer(ibDesc);
			primitive.IndexBuffer->UpdateData(srcPrimitive.Indices.Data(), srcPrimitive.Indices.ByteSize(), 0);
			// texture
			primitive.Texture = TextureResourceMgr::Instance()->GetTexture(srcPrimitive.MaterialFile);
			// aabb
			primitive.AABB = srcPrimitive.AABB;
		}
	}

	void MeshComponent::OnLoad(const Json::Value& val) {
		if(val.HasMember("MeshFile")) {
			SetMeshFile(val["MeshFile"].GetString());
		}
	}

	void MeshComponent::OnAdd() {
		GetScene()->AddComponent<MeshECSComp>(GetEntityID());
	}

	void MeshComponent::OnRemove() {
		GetScene()->RemoveComponent<MeshECSComp>(GetEntityID());
	}

	void MeshComponent::SetMeshFile(const XString& meshFile) {
		m_MeshFile = meshFile;
		MeshECSComp* component = GetScene()->GetComponent<MeshECSComp>(GetEntityID());
		Asset::MeshAsset asset;
		Asset::AssetLoader::LoadProjectAsset(&asset, m_MeshFile.c_str());
		component->BuildFromAsset(asset);
	}

	void MeshRenderSystem::Update(ECSScene* ecsScene, TransformECSComp* transform, MeshECSComp* staticMesh) {
		// update uniform
		RHIDynamicBuffer uniformBuffer = RHI::Instance()->AllocateDynamicBuffer(EBufferFlags::Uniform, sizeof(TransformECSComp::MatrixData), &transform->GetMatrixData(), 0);
		// create draw call
		Object::RenderScene* scene = (Object::RenderScene*)ecsScene;
		Object::RenderCamera* mainCamera = scene->GetMainCamera();
		Object::DirectionalLight* light = scene->GetDirectionalLight();
		Render::DrawCallContext& drawCallContext = scene->GetDrawCallContext();
		for(auto& primitive: staticMesh->Primitives) {
			const Math::AABB3 aabb = primitive.AABB.Transform(transform->GetMatrixData().Transform);
			if(mainCamera->GetFrustum().Cull(aabb)) {
				drawCallContext.PushDrawCall(Render::EDrawCallQueueType::BasePass, [&primitive, uniformBuffer](RHICommandBuffer* cmd) {
					cmd->BindVertexBuffer(primitive.VertexBuffer.Get(), 0, 0);
					cmd->BindIndexBuffer(primitive.IndexBuffer.Get(), 0);
					cmd->SetShaderParam(1, 0, RHIShaderParam::UniformBuffer(uniformBuffer));
					// material albedo
					cmd->SetShaderParam(2, 0, RHIShaderParam::Texture(primitive.Texture));
					RHISampler* defaultSampler = Render::DefaultResources::Instance()->GetDefaultSampler(ESamplerFilter::Bilinear, ESamplerAddressMode::Clamp);
					cmd->SetShaderParam(2, 1, RHIShaderParam::Sampler(defaultSampler));
					cmd->DrawIndexed(primitive.IndexCount, 1, 0, 0, 0);
				});
			}
			// for shadow map
			if(light->GetEnableShadow()) {
				for (uint32 i = 0; i < light->GetCascadeNum(); ++i) {
					const auto& frustum = light->GetFrustum(i);
					if(frustum.Cull(aabb)) {
						auto& dcQueue = light->GetDrawCallQueue(i);
						dcQueue.PushDrawCall([&primitive, uniformBuffer](RHICommandBuffer* cmd) {
							cmd->SetShaderParam(1, 0, RHIShaderParam::UniformBuffer(uniformBuffer));
							cmd->BindVertexBuffer(primitive.VertexBuffer.Get(), 0, 0);
							cmd->BindIndexBuffer(primitive.IndexBuffer.Get(), 0);
							cmd->DrawIndexed(primitive.IndexCount, 1, 0, 0, 0);
						});
					}
				}
			}
		}
	}

	void InstancedDataECSComponent::BuildInstances(const XString& instanceFile) {
		Asset::InstancedMeshAsset::LoadInstanceFile(instanceFile.c_str(), Instances);
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
		auto* com = GetScene()->GetComponent<InstancedDataECSComponent>(GetEntityID());
		com->BuildInstances(file);
	}

	void InstancedMeshRenderSystem::Update(ECSScene* scene, MeshECSComp* meshCom, InstancedDataECSComponent* instanceCom) {

	}
}
