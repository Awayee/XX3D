#include "Math/Public/Geometry.h"

namespace Math {
	AABB2::AABB2() {
	}

	AABB2::AABB2(FVector2 center, FVector2 extent){
		Min = center - extent;
		Max = center + extent;
	}

	FVector2 AABB2::Center() const {
		return 0.5f * (Max + Min);
	}

	FVector2 AABB2::Extent() const {
		return 0.5f * (Max - Min);
	}

	bool AABB2::IsValid() const {
		const FVector2 size = Max - Min;
		return size.X > 0 && size.Y > 0;
	}

	bool AABB2::Contains(FVector2 point) const {
		return FVector2::AllLessOrEqual(point, Max) && FVector2::AllGreaterOrEqual(point, Min);
	}

	bool AABB2::Contains(const AABB2& aabb) const {
		return FVector2::AllLessOrEqual(aabb.Max, Max) && FVector2::AllGreaterOrEqual(aabb.Min, Min);
	}

	bool AABB2::Intersects(const AABB2& aabb) const {
		return FVector2::AllLessOrEqual(Min, aabb.Max) && FVector2::AllGreaterOrEqual(Max, aabb.Min);
	}

	void AABB2::Include(FVector2 newPoint) {
		Min = FVector2::Min(Min, newPoint);
		Max = FVector2::Max(Max, newPoint);
	}

	void AABB2::Merge(const AABB2& aabb) {
		Min = FVector2::Min(Min, aabb.Min);
		Max = FVector2::Max(Max, aabb.Max);
	}

	AABB3::AABB3() = default;

	AABB3::AABB3(const FVector3& center, const FVector3& extent) {
		Min = center - extent;
		Max = center + extent;
	}

	FVector3 AABB3::Center() const {
		return 0.5f * (Max + Min);
	}

	FVector3 AABB3::Extent() const {
		return 0.5f * (Max - Min);
	}

	bool AABB3::IsValid() const {
		const FVector3 size = Max - Min;
		return size.X > 0 && size.Y > 0 && size.Z > 0;
	}

	bool AABB3::Contains(const FVector3& point) const {
		return FVector3::AllLessOrEqual(point, Max) && FVector3::AllGreaterOrEqual(point, Min);
	}

	bool AABB3::Contains(const AABB3& aabb) const {
		return FVector3::AllLessOrEqual(aabb.Max, Max) && FVector3::AllGreaterOrEqual(aabb.Min, Min);
	}

	bool AABB3::Intersects(const AABB3& aabb) const {
		return FVector3::AllLessOrEqual(Min, aabb.Max) && FVector3::AllGreaterOrEqual(Max, aabb.Min);
	}

	void AABB3::Include(const FVector3& newPoint)
	{
		Min = FVector3::Min(Min, newPoint);
		Max = FVector3::Max(Max, newPoint);
	}
	void AABB3::Merge(const AABB3& aabb)
	{
		Min = FVector3::Min(Min, aabb.Min);
		Max = FVector3::Max(Max, aabb.Max);
	}
}
