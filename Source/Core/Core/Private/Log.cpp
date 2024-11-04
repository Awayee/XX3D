#include "Core/Public/Log.h"
#include "Core/Public/Time.h"
#include <cstdarg>

#define LOG_COLOR_RED     "1;31m"
#define LOG_COLOR_YELLOW  "1;33m"
#define LOG_COLOR_BLUE    "1;34m"
#define LOG_COLOR_WHITE   "1;37m"
constexpr int CHAR_BUFFER_NUM = 256;

namespace Log {

	inline const char* GetLogColor(ELogLevel level) {
		switch (level) {
		case ELogLevel::Debug:
			return LOG_COLOR_BLUE "[DEBUG] ";
		case ELogLevel::Info:
			return LOG_COLOR_WHITE "[INFO] ";
		case ELogLevel::Warning:
			return LOG_COLOR_YELLOW "[WARNING] ";
		case ELogLevel::Error:
			return LOG_COLOR_RED "[ERROR] ";
		case ELogLevel::Fatal:
			return LOG_COLOR_RED "[FATAL] ";
		}
		return LOG_COLOR_WHITE;
	}

	void Output(ELogLevel level, const char* fmt, ...) {
		char buf[CHAR_BUFFER_NUM];
		std::va_list args;
		va_start(args, fmt);
		std::vsprintf(buf, fmt, args);
		va_end(args);
		const char* color = GetLogColor(level);
		std::printf("\033[%s%s\033[0m\n", color, buf);
	}

	void OutputWithTime(ELogLevel level, const char* fmt, ...) {
		Duration duration = DurationSceneLaunch();
		char buf[CHAR_BUFFER_NUM];
		std::va_list args;
		va_start(args, fmt);
		std::vsprintf(buf, fmt, args);
		va_end(args);

		// get time format by xx:xx:xxx
		const auto minutes = std::chrono::duration_cast<DurationMinutes<int>>(duration);
		duration -= minutes;
		const auto seconds = std::chrono::duration_cast<DurationSec<int>>(duration);
		duration -= seconds;
		const auto mill = std::chrono::duration_cast<DurationMill<int>>(duration);

		const char* color = GetLogColor(level);
		std::printf("[%02d:%02d.%03d]\033[%s%s\033[0m\n", minutes.count(), seconds.count(), mill.count(), color, buf);
	}
}
