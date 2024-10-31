#pragma once
#include <algorithm>
#include "Core/Public/Defines.h"


inline uint32 Hash32Combine(uint32 A, uint32 B){
	return A ^ (B + 0x9e3779b9 + (A << 6) + (A >> 2));
}

inline uint64 Hash64Combine(uint64 A, uint64 B) {
	static constexpr uint64 P = 0xfff;
	static constexpr uint64 M = 0xffff;
	return A + (B * P) % M;
}

template<typename T> uint64 GetTypeHash64(T val) {
	if constexpr (sizeof(T) == 1) {
		return (uint64)(*(uint8*)&val);
	}
	else if constexpr (sizeof(T) == 2) {
		return (uint64)(*(uint16*)&val);
	}
	else if constexpr (sizeof(T) == 4) {
		return (uint64)(*(uint32*)&val);
	}
	else if constexpr (sizeof(T) == 8) {
		return *(uint64*)&val;
	}
	else if constexpr (sizeof(T) == 16) {
		const uint64 Low = (uint64)val;
		const uint64 High = (uint64)(val >> 64);
		return GetTypeHash64(Low) ^ GetTypeHash64(High);
	}
	return 0;
}

template<typename T> uint64 GetTypeHash64BasedOn(T val, uint64 baseVal) {
	uint64 hs = val;
	if(0 == hs){
		return baseVal;
	}
	if(0 == baseVal) {
		return hs;
	}
	return Hash64Combine(baseVal, hs);
}

template<typename T> uint32 GetTypeHash32(T val) {
	if constexpr(sizeof(T) == 1) {
		return (uint32)(*(uint8*)&val);
	}
	else if constexpr (sizeof(T) == 2) {
		return (uint32)(*(uint16*)&val);
	}
	else if constexpr (sizeof(T) == 4) {
		return *(uint32*)&val;
	}
	else if constexpr(sizeof(T) == 8) {
		const uint64 val64 = *(uint64*)&val;
		return (uint32)val64 + ((uint32)(val64 >> 32) * 23);
	}
	else if constexpr (sizeof(T) == 16){
		const uint64 Low = (uint64)val;
		const uint64 High = (uint64)(val >> 64);
		return GetTypeHash32(Low) ^ GetTypeHash32(High);
	}
	return 0;
}

template<typename T> uint32 GetTypeHash32BasedOn(T val, uint32 baseVal) {
	const uint32 hs = GetTypeHash32(val);
	if(0 == hs) {
		return baseVal;
	}
	if(0==baseVal) {
		return hs;
	}
	return Hash32Combine(baseVal, hs);
}

template<typename T> uint32 DataArrayHash32(const T* data, uint32 size, uint32 baseVal=0) {
	for(uint32 i=0; i<size; ++i) {
		baseVal = Hash32Combine(baseVal, GetTypeHash32(data[i]));
	}
	return baseVal;
}

template<typename T> uint64 DataArrayHash64(const T* data, uint64 size, uint64 baseVal=0) {
	for (uint64 i = 0; i < size; ++i) {
		baseVal = Hash64Combine(baseVal, GetTypeHash64(data[i]));
	}
	return baseVal;
}
