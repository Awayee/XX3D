#include "Objects/Public/Camera.h"
namespace Object {
	Camera::Camera(EProjectiveType type, float aspect, float zNear, float zFar, float param): m_ProjType(type), m_Aspect(aspect), m_Near(zNear), m_Far(zFar), m_Fov(param) {
		m_ViewMatrix = Math::FMatrix4x4::IDENTITY;
		UpdateProjectMatrix();
	}
	void Camera::SetView(const Math::FVector3& eye, const Math::FVector3& at, const Math::FVector3& up)
	{
		m_View.Eye = eye;
		m_View.At = at;
		m_View.Up = up;
		m_ViewMatrix = Math::FMatrix4x4::LookAtMatrix(eye, at, up);
		m_ViewProjectMatrix = m_ProjectMatrix * m_ViewMatrix;
		m_InvViewProjectMatrix = m_ViewProjectMatrix.Inverse();
	}
	Camera::~Camera(){}

	void Camera::UpdateProjectMatrix()
	{
		if (EProjectiveType::Perspective == m_ProjType) {
			m_ProjectMatrix = Math::FMatrix4x4::PerspectiveMatrix(m_Fov, m_Aspect, m_Near, m_Far);
			m_ProjectMatrix[1][1] *= -1.0f;
		}
		else {
			float offsetY = m_Height;
			float offsetX = m_Height * m_Aspect;
			m_ProjectMatrix = Math::FMatrix4x4::OrthographicMatrix(-offsetX, offsetX, -offsetY, offsetY, m_Near, m_Far);
		}
		m_ViewProjectMatrix = m_ProjectMatrix * m_ViewMatrix;
		m_InvViewProjectMatrix = m_ViewProjectMatrix.Inverse();
	}
}