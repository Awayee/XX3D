#include "Objects/Public/RenderObject3D.h"

namespace Engine {
	struct SModelUBO {
		Math::FMatrix4x4 ModelMat;
		Math::FMatrix4x4 InvModelMat;
	};

	RenderObject3D::RenderObject3D(RenderScene* scene): RenderObject(scene) {
		m_Dirty = true;
		m_TransformUniform.CreateForUniform(sizeof(SModelUBO), nullptr);
		// descriptor set
		m_TransformDescs = RHI::Instance()->AllocateDescriptorSet(DescsMgr::Get(DESCS_MODEL));
		m_TransformDescs->SetUniformBuffer(0, m_TransformUniform.Buffer);
	}

	RenderObject3D::~RenderObject3D() {
		m_TransformUniform.Release();
	}

	void RenderObject3D::SetPosition(const Math::FVector3& pos) {
		m_Position = pos;
		m_Dirty = true;
	}

	void RenderObject3D::SetRotation(const Math::FQuaternion& rot) {
		m_Rotation = rot;
		m_Dirty = true;
	}

	void RenderObject3D::SetScale(const Math::FVector3& scale) {
		m_Scale = scale;
		m_Dirty = true;
	}

	void RenderObject3D::Update() {
		if(m_Dirty) {
			m_TransformMat.MakeTransform(m_Position, m_Scale, m_Rotation);
			m_InvTransformMat.MakeInverseTransform(m_Position, m_Scale, m_Rotation);
			m_TransformUniform.UpdateData(&m_TransformMat);
			m_Dirty = false;
		}
	}

	void RenderObject3D::DrawCall(Engine::RCommandBuffer* cmd, Engine::RPipelineLayout* layout) {
		cmd->BindDescriptorSet(layout, m_TransformDescs, 1, Engine::PIPELINE_GRAPHICS);
	}
}
