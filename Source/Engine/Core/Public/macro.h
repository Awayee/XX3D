#pragma once
#include <iostream>


#define ASSERT(x, s)\
	if(!(x))throw s

#define LOG(...)\
	printf(__VA_ARGS__); std::cout << std::endl

#define ERROR(...)\
	printf(__VA_ARGS__); std::cout << std::endl; ASSERT(0, "")
	