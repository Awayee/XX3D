#include "Objects/Public/StaticMesh.h"

namespace Engine {

	struct SModelUBO {
		Math::FMatrix4x4 ModelMat;
		Math::FMatrix4x4 InvModelMat;
	};

	StaticMesh::StaticMesh(const AMeshAsset& meshAsset, RenderScene* scene): RenderObject3D(scene) {
		auto r = RHI::Instance();
		m_Primitives.Resize(meshAsset.Primitives.Size());
		for (uint32 i = 0; i < m_Primitives.Size(); ++i) {
			const auto& primitiveData = meshAsset.Primitives[i];
			auto& primitive = m_Primitives[i];
			primitive.VertexCount = primitiveData.Vertices.Size();
			primitive.IndexCount = primitiveData.Indices.Size();
			// vb
			RHIBufferDesc vbDesc{ BUFFER_FLAG_VERTEX | BUFFER_FLAG_COPY_DST, primitiveData.Vertices.ByteSize(), sizeof(IndexType) };
			primitive.VertexBuffer = r->CreateBuffer(vbDesc);
			primitive.VertexBuffer->UpdateData(primitiveData.Vertices.Data(), primitiveData.Vertices.ByteSize());
			// ib
			RHIBufferDesc ibDesc{ BUFFER_FLAG_INDEX | BUFFER_FLAG_COPY_DST, primitiveData.Indices.ByteSize(), sizeof(IndexType) };
			primitive.IndexBuffer = r->CreateBuffer(ibDesc);
			primitive.IndexBuffer->UpdateData(primitiveData.Indices.Data(), primitiveData.Indices.ByteSize());
		}
	}

	StaticMesh::~StaticMesh(){
	}

	void StaticMesh::CreateDrawCall(Render::DrawCallQueue& queue) {
		RenderObject3D::CreateDrawCall(queue);
		for(auto& primitive: m_Primitives) {
			queue.EmplaceBack().ResetFunc([&primitive](RHICommandBuffer* cmd) {
				cmd->BindVertexBuffer(primitive.VertexBuffer.Get(), 0, 0);
				cmd->BindIndexBuffer(primitive.IndexBuffer.Get(), 0);
				cmd->DrawIndexed(primitive.IndexCount, 1, 0, 0, 0);
			});
		}
	}
}
