#include "Math/Public/Geometry.h"

namespace Math {
	AABB::AABB() = default;

	AABB::AABB(const FVector3& center, const FVector3& extent) : Center(center), Extent(extent) {}

	FVector3 AABB::Min() const {
		return Center - Extent;
	}

	FVector3 AABB::Max() const {
		return Center + Extent;
	}

	void AABB::Include(const FVector3& newPoint)
	{
		const FVector3 min = FVector3::Min(Min(), newPoint);
		const FVector3 max = FVector3::Max(Max(), newPoint);
		Center = (min + max) * 0.5f;
		Extent = (max - min) * 0.5f;
	}
	void AABB::Merge(const AABB& aabb)
	{
		const FVector3 thisMin = FVector3::Min(Min(), aabb.Min());
		const FVector3 thisMax = FVector3::Max(Max(), aabb.Max());
		Center = (thisMin + thisMax) * 0.5f;
		Extent = (thisMax - thisMin) * 0.5f;
	}

	bool AABB::Contain(const FVector3& point) const {
		const FVector3 vec = FVector3::Abs(point - Center);
		return FVector3::AllLessOrEqual(vec, Extent);
	}

	bool AABB::Intersect(const AABB& aabb) const {
		const FVector3 vec = FVector3::Abs(aabb.Center - Center);
		const FVector3 wide = aabb.Extent + Extent;
		return FVector3::AllLessOrEqual(vec, wide);
	}
}
