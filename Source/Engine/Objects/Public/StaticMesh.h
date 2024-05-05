#pragma once
#include "Math/Public/Math.h"
#include "Asset/Public/MeshAsset.h"
#include "RenderObject3D.h"
#include "Core/Public/TUniquePtr.h"
#include "RHI/Public/RHI.h"

namespace Engine {
	// static mesh
	class StaticMesh: public RenderObject3D {
	private:
		TVector<Material*> m_Materials;
		//RHI::RDescriptorSet* m_DescriptorSet;
	public:
		StaticMesh(const AMeshAsset& meshAsset, RenderScene* scene);
		~StaticMesh() override;
	};
}
