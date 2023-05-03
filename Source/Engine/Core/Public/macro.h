#pragma once
#include <iostream>


#define ASSERT(x, s)\
	if(!(x))throw s

#ifdef _DEBUG
#define PRINT(...)\
	printf(__VA_ARGS__); std::cout << std::endl
#else
#define PRINT(...)
#endif

#define LOG(...)\
	printf(__VA_ARGS__); std::cout << std::endl

#define ERROR(...)\
	printf(__VA_ARGS__); std::cout << std::endl; ASSERT(0, "")
	