#include "Objects/Public/StaticMesh.h"

namespace Engine {

	struct SModelUBO {
		Math::FMatrix4x4 ModelMat;
		Math::FMatrix4x4 InvModelMat;
	};

	StaticMesh::StaticMesh(const AMeshAsset& meshAsset, RenderScene* scene): RenderObject3D(scene) {
	}

	StaticMesh::~StaticMesh()
	{
	}
}