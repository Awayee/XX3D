#include "Objects/Public/Camera.h"
#include "Window/Public/EngineWindow.h"
namespace Object {
	Math::FMatrix4x4 CameraView::GetViewMatrix() const {
		return Math::FMatrix4x4::LookAtMatrix(Eye, At, Up);
	}

	void CameraView::CalcAxis(Math::FVector3& outRight, Math::FVector3& outUp, Math::FVector3& outFront) const {
		outFront = (At - Eye).Normalize();
		outRight = Up.Cross(outFront).Normalize();
		outUp = outFront.Cross(outRight);
	}

	CameraProjection::CameraProjection(): ProjType(EProjType::Perspective), Near(0.1f), Far(1000.0f), Fov(1.0f), Aspect(1.0f) {}

	CameraProjection CameraProjection::Perspective(float fov, float aspect, float zNear, float zFar) {
		CameraProjection projection;
		projection.ProjType = EProjType::Perspective;
		projection.Fov = fov;
		projection.Aspect = aspect;
		projection.Near = zNear;
		projection.Far = zFar;
		return projection;
	}

	CameraProjection CameraProjection::Orthographic(float left, float right, float bottom, float top, float zNear, float zFar) {
		CameraProjection projection;
		projection.ProjType = EProjType::Ortho;
		projection.Left = left;
		projection.Right = right;
		projection.Bottom = bottom;
		projection.Top = top;
		projection.Near = zNear;
		projection.Far = zFar;
		return projection;
	}

	Math::FMatrix4x4 CameraProjection::GetProjectMatrix() const {
		if (EProjType::Perspective == ProjType) {
			return Math::FMatrix4x4::PerspectiveMatrix(Fov, Aspect, Near, Far);
		}
		else {
			return Math::FMatrix4x4::OrthographicMatrix(Left, Right, Bottom, Top, Near, Far);
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
			Frustum.Near = { cameraPos + cameraFront * Projection.Near, cameraFront };
			Frustum.Far = { cameraPos + cameraFront * Projection.Far, -cameraFront };
			Frustum.Left = { cameraPos + cameraRight * Projection.Left, cameraRight };
			Frustum.Right = { cameraPos + cameraRight * Projection.Right, -cameraRight };
			Frustum.Bottom = { cameraPos + cameraUp * Projection.Bottom, cameraUp };
			Frustum.Top = { cameraPos + cameraUp * Projection.Top, -cameraUp };
		}
	}

	void FrustumCorner::Build(const CameraView& view, const CameraProjection& proj) {
		if(EProjType::Perspective == proj.ProjType) {
			BuildFromPerspective(view, proj.Near, proj.Far, proj.Fov, proj.Aspect);
		}
		else {
			BuildFormOrtho(view, proj.Near, proj.Far, proj.Left, proj.Right, proj.Bottom, proj.Top);
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
		up = f * tanHalfFov * cameraUp;
		right = f * tanHalfFov * aspect * cameraRight;
		LeftBottomFar = center - right - up;
		RightBottomFar = center + right - up;
		LeftTopFar = center - right + up;
		RightTopFar = center + right + up;
	}

	void FrustumCorner::BuildFormOrtho(const CameraView& view, float n, float f, float left, float right, float bottom, float top) {
		const Math::FVector3 cameraPos = view.Eye;
		Math::FVector3 cameraRight, cameraUp, cameraFront;
		view.CalcAxis(cameraRight, cameraUp, cameraFront);
		const Math::FVector3 nearCenter = cameraPos + n * cameraFront;
		LeftBottomNear = nearCenter + cameraRight * left + cameraUp * bottom;
		RightBottomNear = nearCenter + cameraRight * right + cameraUp * bottom;
		LeftTopNear = nearCenter + cameraRight * left + cameraUp * top;
		RightTopNear = nearCenter + cameraRight * right + cameraUp * top;
		const Math::FVector3 near2Far = (f - n) * cameraFront;
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

	void FrustumCorner::GetSubFrustumCorner(float n, float f, FrustumCorner& out) const {
		Math::FVector3 lbDir = LeftBottomFar - LeftBottomNear;
		Math::FVector3 rbDir = RightBottomFar - RightBottomNear;
		Math::FVector3 ltDir = LeftTopFar - LeftTopNear;
		Math::FVector3 rtDir = RightTopFar - RightTopNear;
		out.LeftBottomNear = LeftBottomNear + n * lbDir;
		out.RightBottomNear = RightBottomNear + n * rbDir;
		out.LeftTopNear = LeftTopNear + n * ltDir;
		out.RightTopNear = RightTopNear + n * rtDir;
		out.LeftBottomFar = LeftBottomNear + f * lbDir;
		out.RightBottomFar = RightBottomNear + f * rbDir;
		out.LeftTopFar = LeftTopNear + f * ltDir;
		out.RightTopFar = RightTopNear + f * rtDir;
	}

	struct CameraUBO {
		Math::FMatrix4x4 View;
		Math::FMatrix4x4 Proj;
		Math::FMatrix4x4 VP;
		Math::FMatrix4x4 InvVP;
		Math::FVector3 CamPos;
	};

	RenderCamera::RenderCamera() {
		// get window size for aspect
		m_ViewMatrix = Math::FMatrix4x4::IDENTITY;
		USize2D size = Engine::EngineWindow::Instance()->GetWindowSize();
		SetAspect((float)size.w / (float)size.h);
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

	void RenderCamera::SetProjectionData(const ProjectionData& data) {
		m_ProjectionData = data;
		UpdateProjection();
		UpdateProjectMatrix();
	}

	void RenderCamera::SetAspect(float aspect) {
		m_Aspect = aspect;
		UpdateProjection();
		UpdateProjectMatrix();
	}

	void RenderCamera::UpdateBuffer() {
		CameraUBO cameraUBO;
		cameraUBO.View = GetViewMatrix();
		cameraUBO.Proj = GetProjectMatrix();
		cameraUBO.VP = GetViewProjectMatrix();
		cameraUBO.InvVP = GetInvViewProjectMatrix();
		cameraUBO.CamPos = GetView().Eye;
		m_Uniform = RHI::Instance()->AllocateDynamicBuffer(EBufferFlags::Uniform, sizeof(CameraUBO), &cameraUBO, 0);
	}

	const RHIDynamicBuffer& RenderCamera::GetUniformBuffer() {
		return m_Uniform;
	}

	RenderCamera::~RenderCamera(){}

	void RenderCamera::UpdateProjection() {
		const auto& data = m_ProjectionData;
		if (EProjType::Ortho == data.ProjType) {
			const float halfHeight = data.HalfHeight;
			const float halfWidth = halfHeight * m_Aspect;
			m_Camera.SetProjection(CameraProjection::Orthographic(-halfWidth, halfWidth, -halfHeight, halfHeight, data.Near, data.Far));
		}
		else if (EProjType::Perspective == data.ProjType) {
			m_Camera.SetProjection(CameraProjection::Perspective(data.Fov, m_Aspect, data.Near, data.Far));
		}
	}

	void RenderCamera::UpdateProjectMatrix(){
		m_ProjectMatrix = m_Camera.Projection.GetProjectMatrix();
		m_ViewProjectMatrix = m_ProjectMatrix * m_ViewMatrix;
		m_InvViewProjectMatrix = m_ViewProjectMatrix.Inverse();
	}
}
