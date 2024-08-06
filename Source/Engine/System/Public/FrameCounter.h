#pragma once
#include "Core/Public/Defines.h"

class FrameCounter {
public:
	static void Update();
	static uint32 GetFrame();
private:
	FrameCounter() = default;
	~FrameCounter() = default;
	static uint32 s_FrameCounter;
};