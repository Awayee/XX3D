#pragma once
#include "Core/Public/Time.h"
#include "Core/Public/Defines.h"

namespace Engine {
	class Timer{
	public:
		static Timer& Instance() { return s_Instance; }
		static float GetFPS() { return Instance().m_FPS; }
		static float GetDeltaTime() { return Instance().m_DeltaTime; }
		static uint32 GetFrame() { return Instance().m_FrameCounter; }
		void Tick();
		void Reset();
	private:
		TimePoint m_NowTime{ NowTimePoint()};
		uint32 m_FrameCounter{ 0U };
		float m_DeltaTime{ 0.0f };
		float m_LastFrameDurationMs{ 0.0f };
		float m_FPS{ 0.0f };
		static Timer s_Instance;
		NON_COPYABLE(Timer);
		NON_MOVEABLE(Timer);
		Timer() = default;
		~Timer() = default;
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