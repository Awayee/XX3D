#pragma once
#include "Math/Public/Math.h"
#include "Math/Public/Geometry.h"
#include "Core/Public/TArray.h"

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
		EProjType ProjType;
		float Near;
		float Far;
		union {
			struct { float Fov, Aspect; };
			struct { float Left, Right, Bottom, Top; };
		};
		CameraProjection();
		static CameraProjection Perspective(float fov, float aspect, float zNear, float zFar);
		static CameraProjection Orthographic(float left, float right, float bottom, float top, float zNear, float zFar);
		Math::FMatrix4x4 GetProjectMatrix() const;
	};

	struct FrustumCorner {
		union {
			struct {
				Math::FVector3 LeftBottomNear, RightBottomNear, LeftTopNear, RightTopNear;
				Math::FVector3 LeftBottomFar, RightBottomFar, LeftTopFar, RightTopFar;
			};
			TStaticArray<Math::FVector3, 8> Corners;
		};
		FrustumCorner() : Corners{} {}
		FrustumCorner(const FrustumCorner& rhs) : Corners(rhs.Corners) {}
		void Build(const CameraView& view, const CameraProjection& proj);
		void BuildFromPerspective(const CameraView& view, float n, float f, float fov, float aspect);
		void BuildFormOrtho(const CameraView& view, float n, float f, float left, float right, float bottom, float top);
		Math::FVector3 GetCenter() const;
		void GetSubFrustumCorner(float n, float f, FrustumCorner& out) const; // n and f is in range [0, 1]
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
}