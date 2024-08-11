#pragma once
#include "Math/Public/Math.h"
#include "Asset/Public/MeshAsset.h"
#include "RenderObject3D.h"
#include "Core/Public/TUniquePtr.h"
#include "RHI/Public/RHI.h"

namespace Object {
	// static mesh
	struct PrimitiveRenderData {
		uint32 VertexCount;
		uint32 IndexCount;
		RHIBufferPtr VertexBuffer;
		RHIBufferPtr IndexBuffer;
		RHITexturePtr Texture;
	};
	class StaticMesh: public RenderObject3D {
	public:
		StaticMesh(const Asset::MeshAsset& meshAsset, RenderScene* scene);
		~StaticMesh() override;
		void CreateDrawCall(Render::DrawCallQueue& queue) override;
	private:
		TVector<PrimitiveRenderData> m_Primitives;
	};
}
