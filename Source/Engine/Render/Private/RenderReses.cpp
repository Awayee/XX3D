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
			rhi->ImmediateCommit([this, &vertexStaging, &indexStaging](Engine::RCommandBuffer* cmd) {
				cmd->CopyBuffer(vertexStaging.Buffer, m_Vertex->Buffer, 0, 0, vertexStaging.Size);
			cmd->CopyBuffer(indexStaging.Buffer, m_Index->Buffer, 0, 0, indexStaging.Size);
				});
			indexStaging.Release();
			vertexStaging.Release();
		}
		else {
			rhi->ImmediateCommit([this, &vertexStaging](Engine::RCommandBuffer* cmd) {
				cmd->CopyBuffer(vertexStaging.Buffer, m_Vertex->Buffer, 0, 0, vertexStaging.Size);
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


	void DrawPrimitive(Engine::RCommandBuffer* cmd, const Primitive* primitive)
	{
		uint32 vertexCount = primitive->GetVertexCount();
		if (0 == vertexCount) {
			return;
		}
		uint32 indexCount = primitive->GetIndexCount();
		if (0 == indexCount) {
			cmd->DrawPrimitive(primitive->GetVertexBuffer(), vertexCount, 1);
		}
		else {
			cmd->DrawPrimitiveIndexed(primitive->GetVertexBuffer(), primitive->GetIndexBuffer(), indexCount, 1);
		}
	}
	Quad::Quad()
	{
		TVector<Math::FVector3> vertices{
			{0, 0, 0}, {1, 0, 0}, {0, 1, 0},
			{0, 1, 0}, {1, 0, 0}, {1, 1, 0}
		};
		uint32 bufferSize = vertices.Size() * sizeof(Math::FVector3);
		m_VertexBuffer.CreateForVertex(bufferSize);
		BufferCommon staging;
		staging.CreateForTransfer(bufferSize, (void*)vertices.Data());
		RHI::Instance()->ImmediateCommit([&staging, this, bufferSize](Engine::RCommandBuffer* cmd) {
			cmd->CopyBuffer(staging.Buffer, m_VertexBuffer.Buffer, 0, 0, bufferSize);
		});
		staging.Release();
	}
	Quad::~Quad()
	{
		m_VertexBuffer.Release();
	}


	void DrawQuad(Engine::RCommandBuffer* cmd, const Quad* quad)
	{
		cmd->DrawPrimitive(quad->GetVertexBuffer(), 6, 1);
	}
}