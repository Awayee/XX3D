#pragma once
#include "Core/Public/Time.h"
#include "Core/Public/Defines.h"

namespace Engine {
	class CTimer{
	public:
		static CTimer* Instance() { return &s_Instance; }
		void Tick();
		void Reset();
		void Pause();
		float GetFPS() { return m_FPS; }
		float GetDeltaTime() { return m_DeltaTime; }
	private:
		TimePoint m_NowTime{ NowTimePoint()};
		uint32 m_FrameCounter{ 0U };
		float m_DeltaTime{ 0.0f };
		bool m_Paused{ false };
		float m_LastFrameDurationMs{ 0.0f };
		float m_FPS{ 0.0f };
		static CTimer s_Instance;
		NON_COPYABLE(CTimer);
		NON_MOVEABLE(CTimer);
		CTimer() = default;
		~CTimer() = default;
	};

	// LOG the time duration of a code segment
	class DurationScope {
	public:
		DurationScope(const char* name);
		~DurationScope();
	private:
		TimePoint m_TimePoint;
		const char* m_Name;
	};
}