#include "Core/Public/Math/MathStructs.h"

namespace Math {
	AxisAlignedBox::AxisAlignedBox(const FVector3& center, const FVector3& extent) : Center(center), Extent(extent) {}

	void AxisAlignedBox::Contain(const FVector3& newPoint)
	{
		FVector3 min = Center - Extent;
		FVector3 max = Center + Extent;
		min = FVector3::Min(min, newPoint);
		max = FVector3::Max(max, newPoint);
		Center = (min + max) * 0.5f;
		Extent = (max - min) * 0.5f;
	}
	void AxisAlignedBox::Combine(const AxisAlignedBox& box)
	{
		FVector3 thisMin = Center - Extent;
		FVector3 thisMax = Center + Extent;
		FVector3 otherMin = box.Center - box.Extent;
		FVector3 otherMax = box.Center + box.Extent;
		thisMin = FVector3::Min(thisMin, otherMin);
		thisMax = FVector3::Max(thisMax, otherMax);
		Center = (thisMin + thisMax) * 0.5f;
		Extent = (thisMax - thisMin) * 0.5f;
	}
}