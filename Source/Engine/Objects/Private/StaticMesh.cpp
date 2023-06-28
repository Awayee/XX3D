#include "Objects/Public/StaticMesh.h"
#include "Objects/Public/Material.h"

namespace Engine {

	struct SModelUBO {
		Math::FMatrix4x4 ModelMat;
		Math::FMatrix4x4 InvModelMat;
	};

	StaticMesh::StaticMesh(const AMeshAsset& meshAsset, RenderScene* scene): RenderObject3D(scene) {
		m_Primitives.Resize(meshAsset.Primitives.Size());
		m_Materials.Resize(m_Primitives.Size());
		for(uint32 i=0; i< m_Primitives.Size(); ++i) {
			const auto& primitiveData = meshAsset.Primitives[i];
			m_Primitives[i].reset(new Primitive(primitiveData.Vertices, primitiveData.Indices));
			m_Materials[i] = primitiveData.MaterialFile.empty()? MaterialMgr::GetDefault() : MaterialMgr::Get(primitiveData.MaterialFile.c_str());
		}
	}

	void StaticMesh::DrawCall(Engine::RCommandBuffer* cmd, Engine::RPipelineLayout* layout)
	{
		RenderObject3D::DrawCall(cmd, layout);
		for(uint32 i=0; i< m_Primitives.Size(); ++i) {
			cmd->BindDescriptorSet(layout, m_Materials[i]->GetDescs(), 2, Engine::PIPELINE_GRAPHICS);
			DrawPrimitive(cmd, m_Primitives[i].get());
		}
	}

	StaticMesh::~StaticMesh()
	{
		m_Primitives.Clear();
	}
}