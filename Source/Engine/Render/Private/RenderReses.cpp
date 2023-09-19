#include "Render/Public/RenderReses.h"
#include "RHI/Public/RHI.h"
#include "Core/Public/File.h"

namespace Engine {
	Primitive::Primitive(const TVector<FVertex>& vertices, const TVector<IndexType>& indices) {

		m_VertexCount = vertices.Size();
		m_IndexCount = indices.Size();
		if (0 == m_VertexCount) {
			return;
		}
		GET_RHI(rhi);
		uint32 bufferSize = m_VertexCount * sizeof(FVertex);
		m_Vertex.reset(new BufferCommon); m_Vertex->CreateForVertex(bufferSize);

		BufferCommon vertexStaging;
		vertexStaging.CreateForTransfer(bufferSize, (void*)vertices.Data());

		if (m_IndexCount > 0) {
			bufferSize = m_IndexCount * sizeof(IndexType);
			m_Index.reset(new BufferCommon); m_Index->CreateForIndex(bufferSize);

			BufferCommon indexStaging;
			indexStaging.CreateForTransfer(bufferSize, (void*)indices.Data());
			rhi->ImmediateSubmit([this, &vertexStaging, &indexStaging](Engine::RHICommandBuffer* cmd) {
				cmd->CopyBufferToBuffer(vertexStaging.Buffer, m_Vertex->Buffer, 0, 0, vertexStaging.Size);
			cmd->CopyBufferToBuffer(indexStaging.Buffer, m_Index->Buffer, 0, 0, indexStaging.Size);
				});
			indexStaging.Release();
			vertexStaging.Release();
		}
		else {
			rhi->ImmediateSubmit([this, &vertexStaging](Engine::RHICommandBuffer* cmd) {
				cmd->CopyBufferToBuffer(vertexStaging.Buffer, m_Vertex->Buffer, 0, 0, vertexStaging.Size);
				});
			vertexStaging.Release();
		}
	}

	Primitive::~Primitive()
	{
		if (m_Index)m_Index->Release();
		if (m_Vertex)m_Vertex->Release();
	}

	void FillVectorInput(TVector<Engine::RVertexInputBinding>& bindings, TVector<Engine::RVertexInputAttribute>& attributes)
	{
		bindings.Resize(1);
		bindings[0] = { 0, sizeof(Math::FVector3), Engine::VERTEX_INPUT_RATE_VERTEX };
		attributes.Resize(1);
		attributes[0] = { 0, Engine::FORMAT_R32G32B32_SFLOAT, 0 };
	}


	void FillVertexInput(TVector<Engine::RVertexInputBinding>& bindings, TVector<Engine::RVertexInputAttribute>& attributes)
	{
		bindings.Resize(1);
		bindings[0] = { 0, sizeof(FVertex)};
		attributes.Resize(4);
		attributes[0] = { 0, Engine::FORMAT_R32G32B32_SFLOAT, 0 };//position
		attributes[1] = { 0, Engine::FORMAT_R32G32B32_SFLOAT, offsetof(FVertex, Normal)};//normal
		attributes[2] = { 0, Engine::FORMAT_R32G32B32_SFLOAT, offsetof(FVertex, Tangent)};// tangent
		attributes[3] = { 0, Engine::FORMAT_R32G32_SFLOAT,    offsetof(FVertex, UV)};//uv
	}


	void DrawPrimitive(Engine::RHICommandBuffer* cmd, const Primitive* primitive)
	{
		uint32 vertexCount = primitive->GetVertexCount();
		if (0 == vertexCount) {
			return;
		}
		uint32 indexCount = primitive->GetIndexCount();
		if (0 == indexCount) {
			cmd->BindVertexBuffer(primitive->GetVertexBuffer(), 0, 0);
			cmd->Draw(vertexCount, 1, 0, 0);
		}
		else {
			cmd->BindVertexBuffer(primitive->GetVertexBuffer(), 0, 0);
			cmd->BindIndexBuffer(primitive->GetIndexBuffer(), 0);
			cmd->DrawIndexed(indexCount, 1, 0, 0, 0);
		}
	}
}