#include "Render/Public/RenderMesh.h"
#include "Render/Public/Material.h"

namespace Engine {

	RenderMesh::RenderMesh(const AMeshAsset* meshAsset, RenderScene* scene): RenderObject(scene) {
		m_Name = meshAsset->Name;
		m_Primitives.resize(meshAsset->Primitives.size());
		m_Materials.resize(m_Primitives.size());
		for(uint32 i=0; i< m_Primitives.size(); ++i) {
			const auto& primitiveData = meshAsset->Primitives[i];
			m_Primitives[i].reset(new Primitive(primitiveData.Vertices, primitiveData.Indices));
			m_Materials[i] = primitiveData.MaterialFile.empty()? MaterialMgr::GetDefault() : MaterialMgr::Get(primitiveData.MaterialFile.c_str());
		}

		m_TransformUniform.CreateForUniform(sizeof(Math::FMatrix4x4), &m_TransformMat);
		// descriptor set
		m_TransformDescs = RHI::Instance()->AllocateDescriptorSet(DescsMgr::Get(DESCS_MODEL));
		m_TransformDescs->UpdateUniformBuffer(0, m_TransformUniform.Buffer);
		// materials
		//RHI::RDescriptorInfo info{};
		//info.buffer = m_TransformUniform.Buffer;
		//info.offset = 0;
		//info.range = m_TransformUniform.Size;
		//RHI::Instance()->UpdateDescriptorSet(m_TransformDescs, 0, 0, 1, RHI::DESCRIPTOR_TYPE_UNIFORM_BUFFER, info);
		//info.buffer = m_MaterialUniform.Buffer;
		//info.offset = 0;
		//info.range = m_MaterialUniform.Size;
		//RHI::Instance()->UpdateDescriptorSet(m_MaterialDescs, 0, 0, 1, RHI::DESCRIPTOR_TYPE_UNIFORM_BUFFER, info);
	}

	void RenderMesh::DrawCall(Engine::RCommandBuffer* cmd, Engine::RPipelineLayout* layout)
	{
		cmd->BindDescriptorSet(layout, m_TransformDescs, 1, Engine::PIPELINE_GRAPHICS);
		for(uint32 i=0; i< m_Primitives.size(); ++i) {
			cmd->BindDescriptorSet(layout, m_Materials[i]->GetDescs(), 2, Engine::PIPELINE_GRAPHICS);
			DrawPrimitive(cmd, m_Primitives[i].get());
		}
	}

	RenderMesh::~RenderMesh()
	{
		m_Primitives.clear();
		m_TransformUniform.Release();
		//RHI::Instance()->FreeDescriptorSet(m_TransformDescs);
	}
}