#pragma once
#include <iostream>


#define ASSERT(x, s)\
	if(!(x))throw s

#ifdef _DEBUG
#define PRINT_DEBUG(...)\
	printf(__VA_ARGS__); std::cout << std::endl
#else
#define PRINT_DEBUG(...)
#endif

#define PRINT(...)\
	printf(__VA_ARGS__); std::cout << std::endl

#define XX_ERROR(...)\
	printf(__VA_ARGS__); std::cout << std::endl; ASSERT(0, "")

#define XX_NODISCARD [[nodiscard]]