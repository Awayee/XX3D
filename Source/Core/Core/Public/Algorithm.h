#pragma once
#include <algorithm>
#include "Core/Public/Defines.h"

template<typename T> uint64 DataArrayHash(const T* data, uint64 size, uint64 startVal=0) {
	static constexpr uint64 P = 0xfff;
	static constexpr uint64 M = 0xffff;
	uint64 p = 1;
	for (uint64 i = 0; i < size; ++i) {
		startVal += (data[i] * p) % M;
		p *= P;
	}
	return startVal;
}