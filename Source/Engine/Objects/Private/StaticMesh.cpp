#include "Objects/Public/StaticMesh.h"
#include "Objects/Public/TextureResource.h"

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
		m_TransformMat.MakeTransform(m_Position, m_Scale, m_Rotation);
		m_InvTransformMat.MakeInverseTransform(m_Position, m_Scale, m_Rotation);
	}

	void StaticMeshComponent::BuildFromAsset(const Asset::MeshAsset& meshAsset) {
		auto r = RHI::Instance();
		m_Primitives.Reserve(meshAsset.Primitives.Size());
		for(auto& srcPrimitive: meshAsset.Primitives) {
			auto& primitive = m_Primitives.EmplaceBack();
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
		}
	}

	void StaticMeshComponent::CreateDrawCall(Render::DrawCallQueue& queue) {
		for(auto& primitive: m_Primitives) {
			queue.EmplaceBack().ResetFunc([&primitive](RHICommandBuffer* cmd) {
				cmd->BindVertexBuffer(primitive.VertexBuffer.Get(), 0, 0);
				cmd->BindIndexBuffer(primitive.IndexBuffer.Get(), 0);
				cmd->DrawIndexed(primitive.IndexCount, 1, 0, 0, 0);
			});
		}
	}

	void MeshRenderSystem::Update(ECSScene* ecsScene, EntityID entityID, TransformComponent* transform, StaticMeshComponent* staticMesh) {
	}
}
