#include "System/Public/FrameCounter.h"


uint32 FrameCounter::s_FrameCounter{ 0 };

void FrameCounter::Update() {
	++s_FrameCounter;
}

uint32 FrameCounter::GetFrame() {
	return s_FrameCounter;
}
