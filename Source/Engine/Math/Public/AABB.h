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
		void Contain(const FVector3& newPoint);
		void Merge(const AABB& box);
	};
}