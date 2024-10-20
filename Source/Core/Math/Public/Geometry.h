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
		static AABB3 MinMax(const FVector3& min, const FVector3& max);
		static AABB3 CenterExtent(const FVector3& center, const FVector3& extent);
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

	enum class EGeometryTest : unsigned char{
		Inner=0,
		Intersected,
		Outer
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
		EGeometryTest TestAABB(const AABB3& aabb) const;
	};

	struct Frustum {
		Plane Near, Far, Left, Right, Bottom, Top;
		bool TestSphereSimple(const BoundingSphere& sphere) const; // return true if sphere in frustum otherwise false
		bool TestAABBSimple(const AABB3& aabb) const;
		EGeometryTest TestAABB(const AABB3& aabb) const;
	};
}