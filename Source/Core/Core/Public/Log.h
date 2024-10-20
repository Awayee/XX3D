#pragma once
#include <iostream>

namespace Log {
	enum class ELogLevel {
		Debug,
		Info,
		Warning,
		Error,
		Fatal
	};

	void Output(ELogLevel level, const char* fmt, ...);
	void OutputWithTime(ELogLevel level, const char* fmt, ...);
}

#define ABORT std::cerr << __FILE__ << ',' << __LINE__ << std::endl; std::abort()

#define CHECK(x)\
	do{\
		if(!(x)){\
			ABORT;\
		}\
	}while (false)

#define ASSERT(x, s)\
	do{\
		if(!(x)){\
			Log::OutputWithTime(Log::ELogLevel::Fatal, s);\
			ABORT;\
		}\
	} while (false)

#ifdef _DEBUG
#define LOG_DEBUG(x, ...) Log::OutputWithTime(Log::ELogLevel::Debug, x, ##__VA_ARGS__)
#else
#define LOG_DEBUG(x, ...)
#endif

#define LOG_INFO(x, ...) Log::OutputWithTime(Log::ELogLevel::Info, x, ##__VA_ARGS__)

#define LOG_WARNING(x, ...) Log::OutputWithTime(Log::ELogLevel::Warning, x, ##__VA_ARGS__)

#define LOG_ERROR(x, ...) Log::OutputWithTime(Log::ELogLevel::Error, x, ##__VA_ARGS__)

#define LOG_FATAL(x, ...) Log::OutputWithTime(Log::ELogLevel::Fatal, x, ##__VA_ARGS__); ABORT