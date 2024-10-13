#include "System/Public/Timer.h"
#include "Core/Public/Log.h"

namespace Engine {

	Timer Timer::s_Instance;

	void Timer::Tick() {
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

	void Timer::Reset() {
		m_FrameCounter = 0;
		m_FPS = 0;
		m_NowTime = NowTimePoint();
	}

	DurationScope::DurationScope(const char* name): m_Name(name) {
		m_TimePoint = NowTimePoint();
	}

	DurationScope::~DurationScope() {
		const float durationMs = GetDurationMill<float>(m_TimePoint, NowTimePoint());
		LOG_DEBUG("[DurationScope] %s %f", m_Name, durationMs);
	}
}
