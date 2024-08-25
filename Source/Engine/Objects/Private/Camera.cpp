#include "Objects/Public/Camera.h"
namespace Object {
	Math::FMatrix4x4 CameraView::GetViewMatrix() const {
		return Math::FMatrix4x4::LookAtMatrix(Eye, At, Up);
	}

	void CameraView::CalcAxis(Math::FVector3& outRight, Math::FVector3& outUp, Math::FVector3& outFront) const {
		outFront = (At - Eye).Normalize();
		outRight = Up.Cross(outFront).Normalize();
		outUp = outFront.Cross(outRight);
	}

	void CameraProjection::SetPerspective(float zNear, float zFar, float fov, float aspect) {
		ProjType = EProjType::Perspective;
		Near = zNear;
		Far = zFar;
		Aspect = aspect;
		Fov = fov;
	}

	void CameraProjection::SetOrtho(float zNear, float zFar, float viewSize, float aspect) {
		ProjType = EProjType::Ortho;
		Near = zNear;
		Far = zFar;
		Aspect = aspect;
		ViewSize = viewSize;
	}

	Math::FMatrix4x4 CameraProjection::GetProjectMatrix() const {
		if (EProjType::Perspective == ProjType) {
			return Math::FMatrix4x4::PerspectiveMatrix(Fov, Aspect, Near, Far);
		}
		else {
			const float right = ViewSize * Aspect;
			const float left = -right;
			const float top = ViewSize;
			const float bottom = -top;
			return Math::FMatrix4x4::OrthographicMatrix(left, right, bottom, top, Near, Far);
		}
	}

	void Camera::SetView(const CameraView& view) {
		View = view;
		UpdateFrustum();
	}

	void Camera::SetProjection(const CameraProjection& projection) {
		Projection = projection;
		UpdateFrustum();
	}

	void Camera::Set(const CameraView& view, const CameraProjection& projection) {
		View = view;
		Projection = projection;
		UpdateFrustum();
	}

	Math::FMatrix4x4 Camera::GetViewProjectMatrix() const {
		return Projection.GetProjectMatrix() * View.GetViewMatrix();
	}

	void Camera::UpdateFrustum() {
		Math::FVector3 cameraPos = View.Eye;
		Math::FVector3 cameraRight, cameraUp, cameraFront;
		View.CalcAxis(cameraRight, cameraUp, cameraFront);
		if (Projection.ProjType == EProjType::Perspective) {
			const float halfHeight = Projection.Far * Math::Tan(Projection.Fov * 0.5f);
			const float halfWidth = halfHeight * Projection.Aspect;
			const Math::FVector3 frontVec = cameraFront * Projection.Far;
			const Math::FVector3 rightVec = cameraRight * halfWidth;
			const Math::FVector3 upVec = cameraUp * halfHeight;
			Frustum.Near = { cameraPos + cameraFront * Projection.Near, cameraFront };
			Frustum.Far = { cameraPos + frontVec, -cameraFront };
			Frustum.Left = { cameraPos, cameraUp.Cross((frontVec - rightVec).Normalize()) };
			Frustum.Right = { cameraPos, (frontVec + rightVec).Normalize().Cross(cameraUp) };
			Frustum.Bottom = { cameraPos, (frontVec - upVec).Normalize().Cross(cameraRight) };
			Frustum.Top = { cameraPos, cameraRight.Cross((frontVec + upVec).Normalize()) };
		}
		else if (Projection.ProjType == EProjType::Ortho) {
			const float halfHeight = Projection.ViewSize;
			const float halfWidth = halfHeight * Projection.Aspect;
			Frustum.Near = { cameraPos + cameraFront * Projection.Near, cameraFront };
			Frustum.Far = { cameraPos + cameraFront * Projection.Far, -cameraFront };
			Frustum.Left = { cameraPos - cameraRight * halfWidth, cameraRight };
			Frustum.Right = { cameraPos + cameraRight * halfWidth, -cameraRight };
			Frustum.Bottom = { cameraPos - cameraUp * halfHeight, cameraUp };
			Frustum.Top = { cameraPos + cameraUp * halfHeight, -cameraUp };
		}
	}

	void FrustumCorner::Build(const CameraView& view, const CameraProjection& proj) {
		if(EProjType::Perspective == proj.ProjType) {
			BuildFromPerspective(view, proj.Near, proj.Far, proj.Fov, proj.Aspect);
		}
		else {
			BuildFormOrtho(view, proj.Near, proj.Far, proj.ViewSize, proj.Aspect);
		}
	}

	void FrustumCorner::BuildFromPerspective(const CameraView& view, float n, float f, float fov, float aspect) {
		const float tanHalfFov = Math::Tan(fov * 0.5f);
		const Math::FVector3 cameraPos = view.Eye;
		Math::FVector3 cameraRight, cameraUp, cameraFront;
		view.CalcAxis(cameraRight, cameraUp, cameraFront);
		Math::FVector3 center = cameraPos + n * cameraFront;
		Math::FVector3 up = n * tanHalfFov * cameraUp;
		Math::FVector3 right = n * tanHalfFov * aspect * cameraRight;
		LeftBottomNear = center - right - up;
		RightBottomNear = center + right - up;
		LeftTopNear = center - right + up;
		RightTopNear = center + right + up;
		center = cameraPos + f * cameraFront;
		up = up = f * tanHalfFov * cameraUp;
		right = n * tanHalfFov * aspect * cameraRight;
		LeftBottomFar = center - right - up;
		RightBottomFar = center + right - up;
		LeftTopFar = center - right + up;
		RightTopFar = center + right + up;
	}

	void FrustumCorner::BuildFormOrtho(const CameraView& view, float n, float f, float viewSize, float aspect) {
		const Math::FVector3 cameraPos = view.Eye;
		Math::FVector3 cameraRight, cameraUp, cameraFront;
		view.CalcAxis(cameraRight, cameraUp, cameraFront);
		const Math::FVector3 center = cameraPos + n * cameraFront;
		const Math::FVector3 up = viewSize * cameraUp;
		const Math::FVector3 right = viewSize * aspect * cameraRight;
		const Math::FVector3 near2Far = (f - n) * cameraFront;
		LeftBottomNear = center - right - up;
		RightBottomNear = center + right - up;
		LeftTopNear = center - right + up;
		RightTopNear = center + right + up;
		LeftBottomFar = LeftBottomNear + near2Far;
		RightBottomFar = RightBottomNear + near2Far;
		LeftTopFar = LeftTopNear + near2Far;
		RightTopFar = RightTopNear + near2Far;
	}

	Math::FVector3 FrustumCorner::GetCenter() const {
		Math::FVector3 center = Math::FVector3::ZERO;
		for(auto& corner: Corners) {
			center += corner;
		}
		center /= 8.0f;
		return center;
	}

	RenderCamera::RenderCamera() {
		m_ViewMatrix = Math::FMatrix4x4::IDENTITY;
		UpdateProjectMatrix();
	}

	void RenderCamera::SetView(const CameraView& view) {
		m_Camera.SetView(view);
		m_ViewMatrix = m_Camera.View.GetViewMatrix();
		m_ViewProjectMatrix = m_ProjectMatrix * m_ViewMatrix;
		m_InvViewProjectMatrix = m_ViewProjectMatrix.Inverse();
	}

	void RenderCamera::SetProjection(const CameraProjection& projection) {
		m_Camera.SetProjection(projection);
		UpdateProjectMatrix();
	}

	void RenderCamera::SetAspect(float aspect) {
		CameraProjection proj = m_Camera.Projection;
		proj.Aspect = aspect;
		m_Camera.SetProjection(proj);
		UpdateProjectMatrix();
	}

	RenderCamera::~RenderCamera(){}

	void RenderCamera::UpdateProjectMatrix(){
		m_ProjectMatrix = m_Camera.Projection.GetProjectMatrix();
		m_ViewProjectMatrix = m_ProjectMatrix * m_ViewMatrix;
		m_InvViewProjectMatrix = m_ViewProjectMatrix.Inverse();
	}
}
