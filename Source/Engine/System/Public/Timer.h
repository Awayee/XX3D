#pragma once
#include "Core/Public/Time.h"

namespace Engine {
	class CTimer {
	private:
		TimePoint m_NowTime{ std::chrono::steady_clock::now() };
		uint32 m_FrameCounter{ 0U };
		float m_DeltaTime;
		bool m_Paused{ false };
		float m_LastFrameDurationMs{ 0.0f };
		float m_FPS{ 0.0f };
	public:
		CTimer() = default;
		CTimer(const CTimer&) = default;
		CTimer(CTimer&&) = default;
		~CTimer() = default;
		void Tick();
		void Reset();
		void Pause();
		uint32 GetFrameCounter() { return m_FrameCounter; }
		float GetFPS() { return m_FPS; }
		float GetTime() { return static_cast<float>(m_NowTime.time_since_epoch().count()); }
		float GetDeltaTime() { return m_DeltaTime; }
	};
}