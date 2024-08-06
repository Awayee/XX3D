#include "System/Public/Timer.h"

namespace Engine {

	void CTimer::Tick() {
		if (m_Paused) {
			return;
		}
		// compute delta time
		TimePoint nowTime = NowTimePoint();
		m_DeltaTime = GetDurationMill<float>(m_NowTime, nowTime);
		m_NowTime = nowTime;

		// compute FPS
		m_LastFrameDurationMs += m_DeltaTime;
		++m_FrameCounter;
		if (m_LastFrameDurationMs > 1000.0f) {
			m_FPS = (float)m_FrameCounter / (m_LastFrameDurationMs * 0.001f);
			m_FrameCounter = 0u;
			m_LastFrameDurationMs = 0.0f;
		}
	}

	void CTimer::Reset() {
		m_FrameCounter = 0;
		m_FPS = 0;
		m_NowTime = NowTimePoint();
	}

	void CTimer::Pause() {
		m_Paused = true;
	}
}
