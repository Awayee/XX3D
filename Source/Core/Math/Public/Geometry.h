#pragma once
#include "Vector.h"
#include "Quaternion.h"

namespace Math {
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
		void Merge(const AABB2& aabb);
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
		void Merge(const AABB3& aabb);
	};
}