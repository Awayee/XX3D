#pragma once
#include "Core/Public/TVector.h"
#include "RHI/Public/RHI.h"
#include "RenderCommon.h"
#include "Core/Public/TPtr.h"
#include "Asset/Public/AssetCommon.h"

namespace Engine {

	class Primitive {
	private:
		uint32 m_VertexCount;
		uint32 m_IndexCount;
		TUniquePtr<BufferCommon> m_Vertex;
		TUniquePtr<BufferCommon> m_Index;
	public:
		Primitive(const TVector<FVertex>& vertices, const TVector<IndexType>& indices);
		Engine::RBuffer* GetVertexBuffer() const { return m_Vertex ? m_Vertex->Buffer : nullptr; }
		Engine::RBuffer* GetIndexBuffer()  const { return m_Index  ? m_Index->Buffer  : nullptr; }
		uint32 GetVertexCount()const { return m_VertexCount; }
		uint32 GetIndexCount()const { return m_IndexCount; }
		~Primitive();
	};

	class Quad {
		BufferCommon m_VertexBuffer;
	public:
		Quad();
		~Quad();
		Engine::RBuffer* GetVertexBuffer() const { return m_VertexBuffer.Buffer; }
	};

	void FillVectorInput(TVector<Engine::RVertexInputBinding>& bindings, TVector<Engine::RVertexInputAttribute>& attributes);

	void FillVertexInput(TVector<Engine::RVertexInputBinding>& bindings, TVector<Engine::RVertexInputAttribute>& attributes);

	void DrawPrimitive(Engine::RCommandBuffer* cmd, const Primitive* primitive);

	void DrawQuad(Engine::RCommandBuffer* cmd, const Quad* quad);
}