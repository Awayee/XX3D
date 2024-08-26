#include "Objects/Public/StaticMesh.h"

#include "Objects/Public/DirectionalLight.h"
#include "Objects/Public/RenderScene.h"
#include "Objects/Public/TextureResource.h"
#include "Render/Public/DefaultResource.h"

namespace Object {
	void TransformComponent::SetPosition(const Math::FVector3& pos) {
		m_Position = pos;
		UpdateMat();
	}

	void TransformComponent::SetRotation(const Math::FQuaternion& rot) {
		m_Rotation = rot;
		UpdateMat();
	}

	void TransformComponent::SetScale(const Math::FVector3& scale) {
		m_Scale = scale;
		UpdateMat();
	}

	void TransformComponent::UpdateMat() {
		m_TransformInfo.TransformMat.MakeTransform(m_Position, m_Scale, m_Rotation);
		m_TransformInfo.InvTransformMat.MakeInverseTransform(m_Position, m_Scale, m_Rotation);
	}

	void StaticMeshComponent::BuildFromAsset(const Asset::MeshAsset& meshAsset) {
		auto r = RHI::Instance();
		Primitives.Reserve(meshAsset.Primitives.Size());
		for(auto& srcPrimitive: meshAsset.Primitives) {
			auto& primitive = Primitives.EmplaceBack();
			primitive.VertexCount = srcPrimitive.Vertices.Size();
			primitive.IndexCount = srcPrimitive.Indices.Size();
			// vb
			RHIBufferDesc vbDesc{ BUFFER_FLAG_VERTEX | BUFFER_FLAG_COPY_DST, srcPrimitive.Vertices.ByteSize(), sizeof(Asset::IndexType) };
			primitive.VertexBuffer = r->CreateBuffer(vbDesc);
			primitive.VertexBuffer->UpdateData(srcPrimitive.Vertices.Data(), srcPrimitive.Vertices.ByteSize(), 0);
			// ib
			RHIBufferDesc ibDesc{ BUFFER_FLAG_INDEX | BUFFER_FLAG_COPY_DST, srcPrimitive.Indices.ByteSize(), sizeof(Asset::IndexType) };
			primitive.IndexBuffer = r->CreateBuffer(ibDesc);
			primitive.IndexBuffer->UpdateData(srcPrimitive.Indices.Data(), srcPrimitive.Indices.ByteSize(), 0);
			// texture
			primitive.Texture = TextureResourceMgr::Instance()->GetTexture(srcPrimitive.MaterialFile);
			// aabb
			primitive.AABB = srcPrimitive.AABB;
		}
		UniformBuffer = r->CreateBuffer({ EBufferFlagBit::BUFFER_FLAG_UNIFORM, sizeof(TransformComponent::TransformInfo), 0});
	}

	void MeshRenderSystem::Update(ECSScene* ecsScene, TransformComponent* transform, StaticMeshComponent* staticMesh) {
		// update uniform
		RHIBuffer* uniformBuffer = staticMesh->UniformBuffer.Get();
		uniformBuffer->UpdateData((void*)(&transform->GetTransformInfo()), sizeof(TransformComponent::TransformInfo), 0);
		// create draw call
		Object::RenderScene* scene = (Object::RenderScene*)ecsScene;
		Object::RenderCamera* mainCamera = scene->GetMainCamera();
		Object::DirectionalLight* light = scene->GetDirectionalLight();
		Render::DrawCallContext& drawCallContext = scene->GetDrawCallContext();
		for(auto& primitive: staticMesh->Primitives) {
			const Math::AABB3 aabb = primitive.AABB.Transform(transform->GetTransformMat());
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

	void MeshRenderSystem::Initialize() {
		Object::RenderScene::RegisterSystemStatic<MeshRenderSystem>();
	}
}
