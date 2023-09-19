#pragma once
#include "Math/Public/Math.h"
#include "Core/Public/TPtr.h"
#include "Asset/Public/MeshAsset.h"
#include "RenderObject3D.h"

namespace Engine {
	// static mesh
	class StaticMesh: public RenderObject3D {
	private:
		TVector<TSharedPtr<Primitive>> m_Primitives;
		TVector<Material*> m_Materials;
		//RHI::RDescriptorSet* m_DescriptorSet;
	public:
		StaticMesh(const AMeshAsset& meshAsset, RenderScene* scene);
		void DrawCall(Engine::RCommandBuffer* cmd, Engine::RPipelineLayout* layout) override;
		~StaticMesh() override;
	};
}