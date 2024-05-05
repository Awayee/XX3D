#include "Objects/Public/RenderObject3D.h"

namespace Engine {
	struct SModelUBO {
		Math::FMatrix4x4 ModelMat;
		Math::FMatrix4x4 InvModelMat;
	};

	RenderObject3D::RenderObject3D(RenderScene* scene): RenderObject(scene) {
		m_Dirty = true;
	}

	RenderObject3D::~RenderObject3D() {
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
			m_Dirty = false;
		}
	}
}
