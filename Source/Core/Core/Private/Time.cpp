#include "Core/Public/Time.h"

static TimePoint s_LaunchTime { NowTimePoint() };

Duration DurationSceneLaunch() {
	return NowTimePoint() - s_LaunchTime;
}
