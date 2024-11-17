#pragma once
#include "Objects/Public/Camera.h"
#include "RHI/Public/RHI.h"
#include "Render/Public/DrawCall.h"
#include "Render/Public/HZBBuilder.h"

namespace Object {

	// TODO subsytem

	struct RenderContext {
		Render::DrawCallQueue CullingQueue;
		Render::DrawCallQueue RenderingQueue;
		RHIBufferPtr CullResultBuffer;
		RHIBufferPtr IndirectCmdBuffer;
		Render::HZBBuilder* HZB{ nullptr };
		void Reset(Render::HZBBuilder* hzb);
	};

	class RenderCameraBase {
	public:
		RenderCameraBase() = default;
		RenderCameraBase(const RenderCameraBase&) = default;
		RenderCameraBase(RenderCameraBase&&) noexcept = default;
		~RenderCameraBase() = default;
		const Camera& GetCamera() const;
		const CameraView& GetView() const;
		const CameraProjection& GetProjection() const;
		const Math::Frustum& GetFrustum() const;
		RenderContext& GetRenderContext();
		const Math::FMatrix4x4& GetViewProjectMatrix() const;
	protected:
		Camera m_Camera;
		Math::FMatrix4x4 m_ViewProjectMatrix;
		RenderContext m_RenderContext;
	};

	// for shadow
	class ShadowCamera: public RenderCameraBase{
	public:
		ShadowCamera() = default;
		const Camera& GetCamera();
		void SetViewProjection(const CameraView& view, const CameraProjection& projection);
		void UpdateBuffer();
		const RHIDynamicBuffer& GetBuffer() const;
	private:
		RHIDynamicBuffer m_Uniform;
	};

	// for main view
	class RenderCamera : public RenderCameraBase {
	public:
		RenderCamera();
		void SetView(const CameraView& view);
		void SetProjection(const CameraProjection& projection);
		void SetView(const Math::FVector3& eye, const Math::FVector3& at, const Math::FVector3& up);
		void SetOrthographicProjection(float left, float right, float bottom, float top, float zNear, float zFar);
		void SetPerspectiveProjection(float fov, float aspect, float zNear, float zFar);
		void SetViewProjection(const CameraView& view, const CameraProjection& projection);
		void SetAspect(float aspect);

		const Camera& GetCamera() const;
		const CameraView& GetView() const;
		const CameraProjection& GetProjection() const;
		const Math::FMatrix4x4& GetViewMatrix() const;
		const Math::FMatrix4x4& GetProjectMatrix() const;
		const Math::FMatrix4x4& GetViewProjectMatrix() const;
		const Math::FMatrix4x4& GetInvViewProjectMatrix() const;

		void UpdateBuffer();
		const RHIDynamicBuffer& GetBuffer() const;
	protected:
		float m_Aspect;
		Math::FMatrix4x4 m_ViewMatrix;
		Math::FMatrix4x4 m_ProjectMatrix;
		Math::FMatrix4x4 m_InvViewProjectMatrix;
		RHIDynamicBuffer m_Uniform;
	};
}