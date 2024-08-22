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
			printf(s);\
			std::abort();\
		}\
	} while (false)

#ifdef _DEBUG
#define LOG_DEBUG(...) printf(__VA_ARGS__); std::cout << std::endl
#else
#define LOG_DEBUG(...)
#endif

#define LOG_INFO(...) printf(__VA_ARGS__); std::cout << std::endl

#define LOG_WARNING(...) printf(__VA_ARGS__); std::cout << std::endl

#define LOG_ERROR(...)\
	printf(__VA_ARGS__); std::cout << std::endl; ASSERT(0, "")