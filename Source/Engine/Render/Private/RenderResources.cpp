#include "Render/Public/RenderResources.h"
#include "RHI/Public/RHI.h"
#include "Core/Public/File.h"

namespace Engine {

	Primitive::Primitive(const TVector<FVertex>& vertices, const TVector<IndexType>& indices)
	{
		m_VertexCount = vertices.size();
		m_IndexCount  = indices.size();
		if(0 == m_VertexCount) {
			return;
		}
		GET_RHI(rhi);
		uint32 bufferSize = m_VertexCount * sizeof(FVertex);
		m_Vertex.reset(new BufferCommon); m_Vertex->CreateForVertex(bufferSize);

		BufferCommon vertexStaging;
		vertexStaging.CreateForTransfer(bufferSize, (void*)vertices.data());

		if(m_IndexCount > 0) {
			bufferSize = m_IndexCount * sizeof(IndexType);
			m_Index.reset(new BufferCommon); m_Index->CreateForIndex(bufferSize);
			
			BufferCommon indexStaging;
			indexStaging.CreateForTransfer(bufferSize, (void*)indices.data());
			rhi->ImmediateCommit([this, &vertexStaging, &indexStaging](RHI::RCommandBuffer* cmd) {
				cmd->CopyBuffer(vertexStaging.Buffer, m_Vertex->Buffer, 0, 0, vertexStaging.Size);
				cmd->CopyBuffer(indexStaging.Buffer, m_Index->Buffer, 0, 0, indexStaging.Size);
			});
			indexStaging.Release();
			vertexStaging.Release();
		}
		else {
			rhi->ImmediateCommit([this, &vertexStaging](RHI::RCommandBuffer* cmd) {
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

	void FillVectorInput(TVector<RHI::RVertexInputBinding>& bindings, TVector<RHI::RVertexInputAttribute>& attributes)
	{
		bindings.resize(1);
		bindings[0] = { 0, sizeof(Math::FVector3), RHI::VERTEX_INPUT_RATE_VERTEX };
		attributes.resize(1);
		attributes[0] = { 0, RHI::FORMAT_R32G32B32_SFLOAT, 0 };
	}


	void FillVertexInput(TVector<RHI::RVertexInputBinding>& bindings, TVector<RHI::RVertexInputAttribute>& attributes)
	{
		bindings.resize(1);
		bindings[0] = { 0, sizeof(Math::FVector3) + sizeof(Math::FVector2) + sizeof(Math::FVector3) + sizeof(Math::FVector3)};
		attributes.resize(4);
		attributes[0] = { 0, RHI::FORMAT_R32G32B32_SFLOAT, 0 };//position
		attributes[1] = { 0, RHI::FORMAT_R32G32_SFLOAT,    sizeof(Math::FVector3)};//uv
		attributes[2] = { 0, RHI::FORMAT_R32G32B32_SFLOAT, sizeof(Math::FVector2) + sizeof(Math::FVector3) };//normal
		attributes[3] = { 0, RHI::FORMAT_R32G32B32_SFLOAT, sizeof(Math::FVector2)};// tangent
	}


	void DrawPrimitive(RHI::RCommandBuffer* cmd, const Primitive* primitive)
	{
		uint32 vertexCount = primitive->GetVertexCount();
		if (0 == vertexCount) {
			return;
		}
		uint32 indexCount = primitive->GetIndexCount();
		if (0 == indexCount) {
			cmd->DrawVertices(primitive->GetVertexBuffer(), vertexCount, 1);
		}
		else {
			cmd->DrawVerticesIndexed(primitive->GetVertexBuffer(), primitive->GetIndexBuffer(), indexCount, 1);
		}
	}
	Quad::Quad()
	{
		TVector<Math::FVector3> vertices{
			{0, 0, 0}, {1, 0, 0}, {0, 1, 0},
			{0, 1, 0}, {1, 0, 0}, {1, 1, 0}
		};
		uint32 bufferSize = vertices.size() * sizeof(Math::FVector3);
		m_VertexBuffer.CreateForVertex(bufferSize);
		BufferCommon staging;
		staging.CreateForTransfer(bufferSize, (void*)vertices.data());
		RHI_INSTANCE->ImmediateCommit([&staging, this, bufferSize](RHI::RCommandBuffer* cmd) {
			cmd->CopyBuffer(staging.Buffer, m_VertexBuffer.Buffer, 0, 0, bufferSize);
		});
		staging.Release();
	}
	Quad::~Quad()
	{
		m_VertexBuffer.Release();
	}


	void DrawQuad(RHI::RCommandBuffer* cmd, const Quad* quad)
	{
		cmd->DrawVertices(quad->GetVertexBuffer(), 6, 1);
	}
}