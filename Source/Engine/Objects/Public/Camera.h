#pragma once
#include "Math/Public/Math.h"
#include "Render/Public/DrawCall.h"

namespace Engine {
	enum EProjectiveType {
		CAMERA_PERSPECTIVE,
		CAMERA_ORTHO
	};
	struct CameraView {
		Math::FVector3 Eye;
		Math::FVector3 At;
		Math::FVector3 Up;
	};

	class Camera {
	public:
		Camera() = default;
		Camera(EProjectiveType type, float aspect, float zNear, float zFar, float param);
		Camera(const Camera&) = default;
		Camera(Camera&&) = default;
		float GetNear() const { return m_Near; }
		void  SetNear(float n) { m_Near = n; UpdateProjectMatrix(); }
		float GetFar() const { return m_Far; }
		void  SetFar(float f) { m_Far = f; UpdateProjectMatrix(); }
		float GetAspect() const { return m_Aspect; }
		void  SetAspect(float a) { m_Aspect = a; UpdateProjectMatrix(); }
		float GetFov() const { return CAMERA_PERSPECTIVE == m_ProjType ? m_Fov : 0.0f; }
		void  SetFov(float fov) { if (CAMERA_PERSPECTIVE == m_ProjType) { m_Fov = fov; UpdateProjectMatrix(); } }
		float GetHeight() const { return CAMERA_ORTHO == m_ProjType ? m_Height : 0.0f; }
		void  SetHeight(float height) { if (CAMERA_ORTHO == m_ProjType) { m_Height = height; UpdateProjectMatrix(); } }
		void  SetView(const Math::FVector3& eye, const Math::FVector3& at, const Math::FVector3& up);
		const CameraView& GetView() { return m_View; }
		const Math::FMatrix4x4& GetViewMatrix() { return m_ViewMatrix; }
		const Math::FMatrix4x4& GetProjectMatrix() { return m_ProjectMatrix; }
		const Math::FMatrix4x4& GetViewProjectMatrix() { return m_ViewProjectMatrix; }
		const Math::FMatrix4x4& GetInvViewProjectMatrix() { return m_InvViewProjectMatrix; }
		Render::DrawCallQueue& GetDrawCallQueue() { return m_DrawCallQueue; }
		~Camera();
	private:
		CameraView m_View;
		// projection
		EProjectiveType m_ProjType;
		float m_Aspect;
		float m_Near;
		float m_Far;
		union {
			float m_Fov;
			float m_Height;
		};
		Math::FMatrix4x4 m_ViewMatrix;
		Math::FMatrix4x4 m_ProjectMatrix;
		Math::FMatrix4x4 m_ViewProjectMatrix;
		Math::FMatrix4x4 m_InvViewProjectMatrix;
		Render::DrawCallQueue m_DrawCallQueue;
		void UpdateProjectMatrix();
	};
}