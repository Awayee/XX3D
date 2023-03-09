#include "Objects/Public/FPSRecorder.h"

namespace Engine {

	void FPSRecorder::Tick() {
		// compute delta time
		TimePoint nowTime = NowTimePoint();
		float deltaTime = GetDurationMill<float>(m_LastTickTime, nowTime);
		m_LastTickTime = nowTime;

		// compute FPS
		m_FPSDurationMs += deltaTime;
		++m_FPSFrameCounter;
		if (m_FPSDurationMs > 1000.0f) {
			m_FPS = (float)m_FPSFrameCounter / (m_FPSDurationMs * 0.001f);
			m_FPSFrameCounter = 0u;
			m_FPSDurationMs = 0.0f;
		}
	}
}
