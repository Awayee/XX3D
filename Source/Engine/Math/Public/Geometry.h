#pragma once
#include "Vector.h"
#include "Quaternion.h"

namespace Math {
	struct AABB {
	public:
		FVector3 Center {0.0f, 0.0f, 0.0f};
		FVector3 Extent {0.0f, 0.0f, 0.0f};
		AABB();
		AABB(const FVector3& center, const FVector3& extent);
		FVector3 Min() const;
		FVector3 Max() const;
		void Include(const FVector3& newPoint);
		void Merge(const AABB& aabb);
		bool Contain(const FVector3& point) const;
		bool Intersect(const AABB& aabb) const;
	};
}