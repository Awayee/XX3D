#pragma once
#include <iostream>

#define CHECK(x)\
	do{\
		if(!(x)){\
			std::abort();\
		}\
	}while (false)

#define ASSERT(x, s)\
	do{\
		if(!(x)){\
			LOG_ERROR(s);\
			std::abort();\
		}\
	} while (false)

#ifdef _DEBUG
#define LOG_DEBUG(x, ...) printf("\033[1;34m"##x##"\033[0m\n", ##__VA_ARGS__)
#else
#define LOG_DEBUG(x, ...)
#endif

#define LOG_INFO(x, ...) printf(x##"\n", ##__VA_ARGS__)

#define LOG_WARNING(x, ...) printf("\033[1;33m"##x##"\033[0m\n", ##__VA_ARGS__)

#define LOG_ERROR(x, ...) printf("\033[1;31m"##x##"\033[0m\n", ##__VA_ARGS__); CHECK(0)