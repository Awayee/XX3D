#pragma once
#include "Math/Public/Math.h"
#include "Core/Public/Defines.h"

namespace Util {

	float RandomF(float min, float max);

	uint32 RandomU32();

	inline float RandomF01() {
		return RandomF(0.0f, 1.0f);
	}

	inline uint32 RandomU32(uint32 min, uint32 max) {
		return RandomU32() % (max - min + 1) + min;
	}

	inline Math::FVector3 RandomVector3(float min, float max) {
		return Math::FVector3{ RandomF(min, max), RandomF(min, max), RandomF(min, max) };
	}

	inline Math::FVector2 RandomVector2InDisk() {
		Math::FVector2 result{ RandomF(-1.0f, 1.0f), RandomF(-1.0f, 1.0f) };
		if (result.LengthSquared() > 1.0f) {
			result.NormalizeSelf();
		}
		return result;
	}
}