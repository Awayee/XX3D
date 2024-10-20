#pragma once
#include <chrono>
#include "Defines.h"

typedef std::chrono::steady_clock::time_point TimePoint;

using Duration = std::chrono::nanoseconds;

template<typename T>
using DurationMill = std::chrono::duration<T, std::milli>;

template<typename T>
using DurationSec = std::chrono::duration<T>;

template<typename T>
using DurationMinutes = std::chrono::duration<T, std::ratio<60>>;

template<typename T>
using DurationHour = std::chrono::duration<T, std::ratio<3600>>;

Duration DurationSceneLaunch();

inline TimePoint NowTimePoint() {
	return std::chrono::steady_clock::now();
}

template<typename T> T NowTimeMs() {
	return static_cast<T>(NowTimePoint().time_since_epoch().count());
}

template<typename T> T GetDurationMill(const TimePoint& start, const TimePoint& end) {
	return std::chrono::duration_cast<DurationMill<T>>(end - start).count();
}

template<typename T> T GetDurationSec(const TimePoint& start, const TimePoint& end) {
	return std::chrono::duration_cast<DurationSec<T>>(end - start).count();
}

inline float NowTimeMsFlt() { return NowTimeMs<float>(); }

inline uint32 NowTimeUint() { return NowTimeMs<uint32>(); }
