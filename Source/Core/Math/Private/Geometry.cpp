#include "Math/Public/Geometry.h"
#include "Math/Public/MathBase.h"

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

	void AABB2::Union(const AABB2& aabb) {
		Min = FVector2::Min(Min, aabb.Min);
		Max = FVector2::Max(Max, aabb.Max);
	}

	AABB3 AABB3::MinMax(const FVector3& min, const FVector3& max) {
		return { min, max };
	}

	AABB3 AABB3::CenterExtent(const FVector3& center, const FVector3& extent) {
		return { center - extent, center + extent };
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
	void AABB3::Union(const AABB3& aabb)
	{
		Min = FVector3::Min(Min, aabb.Min);
		Max = FVector3::Max(Max, aabb.Max);
	}

	void AABB3::TransformSelf(const FMatrix4x4& matrix) {
		// reference: https://www.blurredcode.com/2020/03/aabb%E5%8C%85%E5%9B%B4%E7%9B%92%E5%BF%AB%E9%80%9F%E5%8F%98%E6%8D%A2%E6%96%B9%E5%BC%8F/
		FVector3 col0{ matrix[0][0], matrix[0][1], matrix[0][2] };
		FVector3 col1{ matrix[1][0], matrix[1][1], matrix[1][2] };
		FVector3 col2{ matrix[2][0], matrix[2][1], matrix[2][2] };
		FVector3 col3{ matrix[3][0], matrix[3][1], matrix[3][2] };
		FVector3 xMin = col0 * Min.X;
		FVector3 xMax = col0 * Max.X;
		FVector3 yMin = col1 * Min.Y;
		FVector3 yMax = col1 * Max.Y;
		FVector3 zMin = col2 * Min.Z;
		FVector3 zMax = col2 * Max.Z;
		float w = matrix[3][3];
		Min = FVector3::Min(xMin, xMax) + FVector3::Min(yMin, yMax) + FVector3::Min(zMin, zMax) + col3;
		Max = FVector3::Max(xMin, xMax) + FVector3::Max(yMin, yMax) + FVector3::Max(zMin, zMax) + col3;
		Min /= w;
		Max /= w;
	}

	AABB3 AABB3::Transform(const FMatrix4x4& matrix) const {
		AABB3 aabb = *this;
		aabb.TransformSelf(matrix);
		return aabb;
	}

	bool BoundingSphere::Contains(const FVector3& point) const {
		return Center.DistanceSquared(point) < (Radius * Radius);
	}

	bool BoundingSphere::Contains(const BoundingSphere& bs) const {
		const float radiusSub = Radius - bs.Radius;
		if(radiusSub < 0.0f) {
			return false;
		}
		const float distanceSq = Center.DistanceSquared(bs.Center);
		return distanceSq <= radiusSub * radiusSub;

	}

	bool BoundingSphere::Intersects(const BoundingSphere& bs) const {
		const float distanceSq = Center.DistanceSquared(bs.Center);
		const float radiusSum = Radius + bs.Radius;
		return distanceSq <= radiusSum * radiusSum;
	}

	void BoundingSphere::Include(const FVector3& newPoint) {
		const float distanceSq = Center.DistanceSquared(newPoint);
		if(distanceSq > Radius * Radius) {
			Radius = Math::FSqrt(distanceSq);
		}
	}

	void BoundingSphere::Merge(const BoundingSphere& bs) {
		if(!Contains(bs)) {
			Radius = Center.Distance(bs.Center) + bs.Radius;
		}
	}

	Plane::Plane(const FVector3& point, const FVector3& normal) {
		Normal = normal;
		Distance = normal.Dot(point);
	}

	float Plane::SignedDistance(const FVector3& point) const {
		return Normal.Dot(point) - Distance;
	}

	bool Plane::IsOnOrForward(const AABB3& aabb) const {
		// reference https://gdbooks.gitbooks.io/3dcollisions/content/Chapter2/static_aabb_plane.html
		const FVector3 aabbCenter = aabb.Center();
		const FVector3 aabbExtent = aabb.Extent();
		const float r = aabbExtent.X * Math::Abs(Normal.X) + aabbExtent.Y * Math::Abs(Normal.Y) + aabbExtent.Z * Math::Abs(Normal.Z);
		return SignedDistance(aabbCenter) >= -r;
	}

	bool Plane::IsOnOrForward(const BoundingSphere& bs) const {
		return SignedDistance(bs.Center) > -bs.Radius;
	}

	bool Frustum::Cull(const BoundingSphere& sphere) const {
		return (
			Near.IsOnOrForward(sphere) &&
			Far.IsOnOrForward(sphere) &&
			Left.IsOnOrForward(sphere) &&
			Right.IsOnOrForward(sphere) &&
			Top.IsOnOrForward(sphere) &&
			Bottom.IsOnOrForward(sphere));
	}

	bool Frustum::Cull(const AABB3& aabb) const {
		return (
			Near.IsOnOrForward(aabb) &&
			Far.IsOnOrForward(aabb) &&
			Left.IsOnOrForward(aabb) &&
			Right.IsOnOrForward(aabb) &&
			Top.IsOnOrForward(aabb) &&
			Bottom.IsOnOrForward(aabb));
	}
}
