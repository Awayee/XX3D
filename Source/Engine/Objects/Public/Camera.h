#pragma once
#include "Math/Public/Math.h"
#include "Math/Public/Geometry.h"
#include "Asset/Public/AssetCommon.h"
#include "RHI/Public/RHI.h"

namespace Object {
	enum class EProjType : int{
		Perspective = 0,
		Ortho
	};
	// the struct for getting
	struct CameraView {
		Math::FVector3 Eye;
		Math::FVector3 At;
		Math::FVector3 Up;
		Math::FMatrix4x4 GetViewMatrix() const;
		void CalcAxis(Math::FVector3& outRight, Math::FVector3& outUp, Math::FVector3& outFront) const;
	};

	struct CameraProjection {
		EProjType ProjType{ EProjType::Perspective };
		float Near{ 1.0f };
		float Far{ 100.0f };
		float Aspect{ 1.0f };
		float Fov{ 1.0f }; // vertical fov
		float ViewSize{ 1.0f };// vertical view size
		void SetPerspective(float zNear, float zFar, float fov, float aspect);
		void SetOrtho(float zNear, float zFar, float viewSize, float aspect);
		Math::FMatrix4x4 GetProjectMatrix() const;
	};

	struct Camera {
	public:
		CameraView View;
		CameraProjection Projection;
		Math::Frustum Frustum;
		void SetView(const CameraView& view);
		void SetProjection(const CameraProjection& projection);
		void Set(const CameraView& view, const CameraProjection& projection);
		Math::FMatrix4x4 GetViewProjectMatrix() const;
	private:
		void UpdateFrustum();
	};

	union FrustumCorner {
		struct {
			Math::FVector3 LeftBottomNear, RightBottomNear, LeftTopNear, RightTopNear;
			Math::FVector3 LeftBottomFar, RightBottomFar, LeftTopFar, RightTopFar;
		};
		TStaticArray<Math::FVector3, 8> Corners;
		FrustumCorner() {}
		void Build(const CameraView& view, const CameraProjection& proj);
		void BuildFromPerspective(const CameraView& view, float n, float f, float fov, float aspect);
		void BuildFormOrtho(const CameraView& view, float n, float f, float viewSize, float aspect);
		Math::FVector3 GetCenter() const;
	};

	// cached the matices for rendering
	class RenderCamera {
	public:
		RenderCamera();
		RenderCamera(const RenderCamera&) = default;
		RenderCamera(RenderCamera&&) noexcept = default;
		void  SetView(const CameraView& view);
		const CameraView& GetView() const { return m_Camera.View; }
		void  SetProjection(const CameraProjection& projection);
		const CameraProjection& GetProjection() const { return m_Camera.Projection; }
		void  SetAspect(float aspect);
		const Math::Frustum& GetFrustum()const { return m_Camera.Frustum; }
		const Math::FMatrix4x4& GetViewMatrix() { return m_ViewMatrix; }
		const Math::FMatrix4x4& GetProjectMatrix() { return m_ProjectMatrix; }
		const Math::FMatrix4x4& GetViewProjectMatrix() { return m_ViewProjectMatrix; }
		const Math::FMatrix4x4& GetInvViewProjectMatrix() { return m_InvViewProjectMatrix; }
		void UpdateBuffer();
		const RHIDynamicBuffer& GertUniformBuffer();
		~RenderCamera();
	private:
		Camera m_Camera;
		Math::FMatrix4x4 m_ViewMatrix;
		Math::FMatrix4x4 m_ProjectMatrix;
		Math::FMatrix4x4 m_ViewProjectMatrix;
		Math::FMatrix4x4 m_InvViewProjectMatrix;
		RHIDynamicBuffer m_Uniform;
		void UpdateProjectMatrix();
	};
}