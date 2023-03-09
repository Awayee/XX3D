#include "Render/Public/RenderMesh.h"
#include "Render/Public/Material.h"

namespace Engine {
	RenderMesh::RenderMesh(const TVector<SPrimitiveData>& primitives, RenderScene* scene) : RenderObject(scene)
	{
		LoadPrimitives(primitives);
	}

	void RenderMesh::LoadPrimitives(const TVector<SPrimitiveData>& primitives)
	{
		// primitives
		m_Primitives.resize(primitives.size());
		m_Materials.resize(primitives.size());
		for (uint32 i = 0; i < primitives.size(); ++i) {
			auto& primitiveData = primitives[i];
			m_Primitives[i].reset(new Primitive(primitiveData.Vertices, primitiveData.Indices));
			if(!primitiveData.Textures.empty()) {
				m_Materials[i] = MaterialMgr::Get(primitiveData.Textures[0].c_str());
			}
			else {
				m_Materials[i] = MaterialMgr::GetDefault();
			}
		}

		// uniform
		m_TransformUniform.CreateForUniform(sizeof(Math::FMatrix4x4), &m_TransformMat);
		// descriptor set
		m_TransformDescs = RHI_INSTANCE->AllocateDescriptorSet(DescsMgr::Get(DESCS_MODEL));
		m_TransformDescs->UpdateUniformBuffer(0, m_TransformUniform.Buffer);
		// materials
		//RHI::RDescriptorInfo info{};
		//info.buffer = m_TransformUniform.Buffer;
		//info.offset = 0;
		//info.range = m_TransformUniform.Size;
		//RHI_INSTANCE->UpdateDescriptorSet(m_TransformDescs, 0, 0, 1, RHI::DESCRIPTOR_TYPE_UNIFORM_BUFFER, info);
		//info.buffer = m_MaterialUniform.Buffer;
		//info.offset = 0;
		//info.range = m_MaterialUniform.Size;
		//RHI_INSTANCE->UpdateDescriptorSet(m_MaterialDescs, 0, 0, 1, RHI::DESCRIPTOR_TYPE_UNIFORM_BUFFER, info);
	}

	void RenderMesh::DrawCall(RHI::RCommandBuffer* cmd, RHI::RPipelineLayout* layout)
	{
		cmd->BindDescriptorSet(layout, m_TransformDescs, 1, RHI::PIPELINE_GRAPHICS);
		for(uint32 i=0; i< m_Primitives.size(); ++i) {
			cmd->BindDescriptorSet(layout, m_Materials[i]->GetDescs(), 2, RHI::PIPELINE_GRAPHICS);
			DrawPrimitive(cmd, m_Primitives[i].get());
		}
	}

	RenderMesh::~RenderMesh()
	{
		m_Primitives.clear();
		m_TransformUniform.Release();
		//RHI_INSTANCE->FreeDescriptorSet(m_TransformDescs);
	}
}