#include "Objects/Public/RenderCamera.h"
#include "Window/Public/EngineWindow.h"

namespace Object {

	struct CameraUBO {
		Math::FMatrix4x4 View;
		Math::FMatrix4x4 Proj;
		Math::FMatrix4x4 VP;
		Math::FMatrix4x4 InvVP;
		Math::FVector3 CamPos;
	};

	void RenderContext::Reset(Render::HZBBuilder* hzb) {
		CullingQueue.Reset();
		RenderingQueue.Reset();
		HZB = hzb;
	}

	const Camera& RenderCameraBase::GetCamera() const {
		return m_Camera;
	}

	const CameraView& RenderCameraBase::GetView() const {
		return m_Camera.View;
	}

	const CameraProjection& RenderCameraBase::GetProjection() const {
		return m_Camera.Projection;
	}

	const Math::Frustum& RenderCameraBase::GetFrustum() const {
		return m_Camera.Frustum;
	}

	RenderContext& RenderCameraBase::GetRenderContext() {
		return m_RenderContext;
	}

	const Math::FMatrix4x4& RenderCameraBase::GetViewProjectMatrix() const {
		return m_ViewProjectMatrix;
	}

	const Camera& ShadowCamera::GetCamera() {
		return m_Camera;
	}

	void ShadowCamera::SetViewProjection(const CameraView& view, const CameraProjection& projection) {
		m_Camera.Set(view, projection);
		m_ViewProjectMatrix = m_Camera.GetViewProjectMatrix();
	}

	void ShadowCamera::UpdateBuffer() {
		m_Uniform = RHI::Instance()->AllocateDynamicBuffer(EBufferFlags::Uniform, sizeof(Math::FMatrix4x4), &m_ViewProjectMatrix, 0);
	}

	const RHIDynamicBuffer& ShadowCamera::GetBuffer() const {
		return m_Uniform;
	}

	RenderCamera::RenderCamera() {
		USize2D windowSize = Engine::EngineWindow::Instance()->GetWindowSize();
		float aspect = (float)windowSize.w / (float)windowSize.h;
		SetAspect(aspect);
	}

	void RenderCamera::SetView(const CameraView& view) {
		m_Camera.SetView(view);
		m_ViewMatrix = m_Camera.View.GetViewMatrix();
		m_ViewProjectMatrix = m_ProjectMatrix * m_ViewMatrix;
		m_InvViewProjectMatrix = m_ViewProjectMatrix.Inverse();
	}

	void RenderCamera::SetProjection(const CameraProjection& projection) {
		m_Camera.SetProjection(projection);
		m_ProjectMatrix = m_Camera.Projection.GetProjectMatrix();
		m_ViewProjectMatrix = m_ProjectMatrix * m_ViewMatrix;
		m_InvViewProjectMatrix = m_ViewProjectMatrix.Inverse();
	}

	void RenderCamera::SetView(const Math::FVector3& eye, const Math::FVector3& at, const Math::FVector3& up) {
		SetView(CameraView{ eye, at, up });
	}

	void RenderCamera::SetOrthographicProjection(float left, float right, float bottom, float top, float zNear, float zFar) {
		SetProjection(CameraProjection::Orthographic(left, right, bottom, top, zNear, zFar));
	}

	void RenderCamera::SetPerspectiveProjection(float fov, float aspect, float zNear, float zFar) {
		SetProjection(CameraProjection::Perspective(fov, aspect, zNear, zFar));
	}

	void RenderCamera::SetViewProjection(const CameraView& view, const CameraProjection& projection) {
		m_Camera.Set(view, projection);
		m_ViewMatrix = m_Camera.View.GetViewMatrix();
		m_ProjectMatrix = m_Camera.Projection.GetProjectMatrix();
		m_ViewProjectMatrix = m_ProjectMatrix * m_ViewMatrix;
		m_InvViewProjectMatrix = m_ViewProjectMatrix.Inverse();
	}

	void RenderCamera::SetAspect(float aspect) {
		m_Aspect = aspect;

		const CameraProjection& data = m_Camera.Projection;
		if (EProjType::Ortho == data.ProjType) {
			const float halfHeight = (data.Top - data.Bottom) * 0.5f;
			const float halfWidth = halfHeight * m_Aspect;
			m_Camera.SetProjection(CameraProjection::Orthographic(-halfWidth, halfWidth, -halfHeight, halfHeight, data.Near, data.Far));
		}
		else if (EProjType::Perspective == data.ProjType) {
			m_Camera.SetProjection(CameraProjection::Perspective(data.Fov, m_Aspect, data.Near, data.Far));
		}
		m_ProjectMatrix = m_Camera.Projection.GetProjectMatrix();
		m_ViewProjectMatrix = m_ProjectMatrix * m_ViewMatrix;
		m_InvViewProjectMatrix = m_ViewProjectMatrix.Inverse();
	}

	const Camera& RenderCamera::GetCamera() const {
		return m_Camera;
	}

	const CameraView& RenderCamera::GetView() const {
		return m_Camera.View;
	}

	const CameraProjection& RenderCamera::GetProjection() const {
		return m_Camera.Projection;
	}

	const Math::FMatrix4x4& RenderCamera::GetViewMatrix() const {
		return m_ViewMatrix;
	}

	const Math::FMatrix4x4& RenderCamera::GetProjectMatrix() const {
		return m_ProjectMatrix;
	}

	const Math::FMatrix4x4& RenderCamera::GetViewProjectMatrix() const {
		return m_ViewProjectMatrix;
	}

	const Math::FMatrix4x4& RenderCamera::GetInvViewProjectMatrix() const {
		return m_InvViewProjectMatrix;
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

	const RHIDynamicBuffer& RenderCamera::GetBuffer() const {
		return m_Uniform;
	}
}
