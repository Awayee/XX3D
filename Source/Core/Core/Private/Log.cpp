#include "Core/Public/Log.h"
#include "Core/Public/Time.h"
#include <cstdarg>

#define LOG_COLOR_RED     "\033[1;31m"
#define LOG_COLOR_YELLOW  "\033[1;33m"
#define LOG_COLOR_BLUE    "\033[1;34m"
#define LOG_COLOR_WHITE   "\033[1;37m"
#define LOG_COLOR_END     "\033[0m"

namespace Log {

	inline void PrintTime(Duration duration) {
		auto minites = std::chrono::duration_cast<DurationMinutes<int>>(duration);
		duration -= minites;
		auto seconds = std::chrono::duration_cast<DurationSec<int>>(duration);
		duration -= seconds;
		auto mill = std::chrono::duration_cast<DurationMill<int>>(duration);
		std::printf("[%02d:%02d.%03d]", minites.count(), seconds.count(), mill.count());
	}

	inline void BeginLog(ELogLevel level) {
		switch (level) {
		case ELogLevel::Debug:
			std::cout <<   LOG_COLOR_BLUE "[DEBUG] ";
			break;
		case ELogLevel::Info:
			std::cout <<  LOG_COLOR_WHITE "[INFO] ";
			break;
		case ELogLevel::Warning:
			std::cout << LOG_COLOR_YELLOW "[WARNING] ";
			break;
		case ELogLevel::Error:
			std::cout <<    LOG_COLOR_RED "[ERROR] ";
			break;
		case ELogLevel::Fatal:
			std::cout <<    LOG_COLOR_RED "[FATAL] ";
			break;
		}
	}

	inline void EndLog() {
		std::cout << LOG_COLOR_END << std::endl;
	}

	void Output(ELogLevel level, const char* fmt, ...) {
		BeginLog(level);
		std::va_list args;
		va_start(args, fmt);
		std::vprintf(fmt, args);
		va_end(args);
		EndLog();
	}

	void OutputWithTime(ELogLevel level, const char* fmt, ...) {
		PrintTime(DurationSceneLaunch());
		BeginLog(level);
		std::va_list args;
		va_start(args, fmt);
		std::vprintf(fmt, args);
		va_end(args);
		EndLog();
	}
}
