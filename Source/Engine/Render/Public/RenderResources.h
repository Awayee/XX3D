#pragma once
#include "Core/Public/Container.h"
#include "Core/Public/Math/Math.h"
#include "RHI/Public/RHI.h"
#include "RenderCommon.h"
#include "Core/Public/SmartPointer.h"
#include "Resource/Public/AssetData.h"

namespace Engine {

	class Primitive {
	private:
		uint32 m_VertexCount;
		uint32 m_IndexCount;
		TUniquePtr<BufferCommon> m_Vertex;
		TUniquePtr<BufferCommon> m_Index;
	public:
		Primitive(const TVector<FVertex>& vertices, const TVector<IndexType>& indices);
		RHI::RBuffer* GetVertexBuffer() const { return m_Vertex ? m_Vertex->Buffer : nullptr; }
		RHI::RBuffer* GetIndexBuffer()  const { return m_Index  ? m_Index->Buffer  : nullptr; }
		uint32 GetVertexCount()const { return m_VertexCount; }
		uint32 GetIndexCount()const { return m_IndexCount; }
		~Primitive();
	};

	class Quad {
		BufferCommon m_VertexBuffer;
	public:
		Quad();
		~Quad();
		RHI::RBuffer* GetVertexBuffer() const { return m_VertexBuffer.Buffer; }
	};

	void FillVectorInput(TVector<RHI::RVertexInputBinding>& bindings, TVector<RHI::RVertexInputAttribute>& attributes);

	void FillVertexInput(TVector<RHI::RVertexInputBinding>& bindings, TVector<RHI::RVertexInputAttribute>& attributes);

	void DrawPrimitive(RHI::RCommandBuffer* cmd, const Primitive* primitive);

	void DrawQuad(RHI::RCommandBuffer* cmd, const Quad* quad);
}