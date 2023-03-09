#pragma once
#include "Core/Public/Time.h"

namespace Engine {
	class FPSRecorder {
	private:
		TimePoint m_LastTickTime{ NowTimePoint() };
		uint32 m_FPSFrameCounter{ 0U };
		float m_FPSDurationMs{ 0.0f };
		float m_FPS{ 0.0f };
	public:
		FPSRecorder() = default;
		~FPSRecorder() = default;
		void Tick();
		float GetFPS() { return m_FPS; }
	};	
}