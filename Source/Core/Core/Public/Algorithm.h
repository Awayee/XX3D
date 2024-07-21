#pragma once
#include <algorithm>

inline size_t ByteHash(const unsigned char* data, size_t size) {
	static constexpr size_t P = 0xfff;
	static constexpr size_t M = 0xffff;
	size_t hs = 0;
	size_t p = 1;
	for(size_t i=0; i < size; ++i) {
		hs += (data[i] * p) % M;
		p *= P;
	}
	return hs;
}