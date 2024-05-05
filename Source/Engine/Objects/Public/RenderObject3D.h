#pragma once
#include "RenderScene.h"
#include "Math/Public/Matrix.h"

namespace Engine {
	class RenderObject3D: public RenderObject {
	private:
		Math::FVector3 m_Position          {0,0,0};
		Math::FQuaternion m_Rotation       {0,0,0,1};
		Math::FVector3 m_Scale             {1,1,1};

		Math::FMatrix4x4 m_TransformMat    {Math::FMatrix4x4::IDENTITY};
		Math::FMatrix4x4 m_InvTransformMat {Math::FMatrix4x4::IDENTITY};
		bool m_Dirty;
	public:
		RenderObject3D() = delete;
		RenderObject3D(RenderScene* scene);
		virtual ~RenderObject3D();
		void SetPosition(const Math::FVector3& pos);
		void SetRotation(const Math::FQuaternion& rot);
		void SetScale(const Math::FVector3& scale);
		_NODISCARD const Math::FVector3& GetPosition() const { return m_Position; }
		_NODISCARD const Math::FQuaternion& GetRotation() const { return m_Rotation; }
		_NODISCARD const Math::FVector3& GetScale() const { return m_Scale; }

		virtual void Update() override;
	};
}
