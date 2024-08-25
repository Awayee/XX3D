#pragma once
#include "Vector.h"
#include "Matrix.h"

namespace Math {
	// Axis aligned bounding box
	struct AABB2 {
		FVector2 Min{0.0f, 0.0f};
		FVector2 Max{0.0f, 0.0f};
		AABB2();
		AABB2(FVector2 center, FVector2 extent);
		FVector2 Center() const;
		FVector2 Extent() const;
		bool IsValid() const;
		bool Contains(FVector2 point) const;
		bool Contains(const AABB2& aabb) const;
		bool Intersects(const AABB2& aabb) const;
		void Include(FVector2 newPoint);
		void Union(const AABB2& aabb);
	};

	struct AABB3 {
		FVector3 Min {0.0f, 0.0f, 0.0f};
		FVector3 Max {0.0f, 0.0f, 0.0f};
		AABB3();
		AABB3(const FVector3& center, const FVector3& extent);
		FVector3 Center() const;
		FVector3 Extent() const;
		bool IsValid() const;
		bool Contains(const FVector3& point) const;
		bool Contains(const AABB3& aabb) const;
		bool Intersects(const AABB3& aabb) const;
		void Include(const FVector3& newPoint);
		void Union(const AABB3& aabb);
		void TransformSelf(const FMatrix4x4& matrix);
		AABB3 Transform(const FMatrix4x4& matrix) const;
	};

	struct BoundingSphere {
		FVector3 Center{ 0.0f, 0.0f, 0.0f };
		float Radius{ 0.0f };
		bool Contains(const FVector3& point) const;
		bool Contains(const BoundingSphere& bs) const;
		bool Intersects(const BoundingSphere& bs) const;
		void Include(const FVector3& newPoint);
		void Merge(const BoundingSphere& bs);
	};

	struct Plane {
		FVector3 Normal {0.0f, 1.0f, 0.0f};
		float Distance{ 0.0f };
		// the normal in constructor must be normalized
		Plane() = default;
		Plane(const FVector3& point, const FVector3& normal);

		float SignedDistance(const FVector3& point) const;
		bool IsOnOrForward(const AABB3& aabb) const;
		bool IsOnOrForward(const BoundingSphere& bs)const;
	};

	struct Frustum {
		Plane Near, Far, Left, Right, Bottom, Top;

		bool Cull(const BoundingSphere& sphere) const;
		bool Cull(const AABB3& aabb) const;
	};
}