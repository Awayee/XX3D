#pragma once
#include "Vector.h"
#include "Quaternion.h"

namespace Math {
	struct AxisAlignedBox {
	public:
		FVector3 Center {0.0f, 0.0f, 0.0f};
		FVector3 Extent {0.0f, 0.0f, 0.0f};
		AxisAlignedBox() = default;
		AxisAlignedBox(const FVector3& center, const FVector3& extent);
		void Contain(const FVector3& newPoint);
		void Combine(const AxisAlignedBox& box);
	};

	struct Transform {
		FVector3 Position;
		FVector3 Scale;
		FQuaternion Rotation;
	};
}