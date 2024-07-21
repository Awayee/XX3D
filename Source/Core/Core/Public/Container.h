#pragma once
#include <unordered_map>
#include <map>
#include "TArray.h"
#include <algorithm>
#include <string>
#include <unordered_set>
#include <set>
#include <initializer_list>
#include "Defines.h"


struct cmp {
	bool operator()(const char* s1, const char* s2) const {
		return std::strcmp(s1, s2) == 0;
	}
};

struct hs {
	uint64 operator()(const char* s) const {
		unsigned long n = 0;
		for (; *s; s++) {
			n = 5 * n + *s;
		}
		return (uint64)n;
	}
};
template<class T>
using TStrMap = std::unordered_map<const char*, T, hs, cmp>;

//template<class T>
//using TVector =  std::vector<T>;

template<typename T1, typename T2>
using TPair = std::pair<T1, T2>;

template<typename T>
using TUnorderedSet = std::unordered_set<T>;

template<typename T>
using TSet = std::set<T>;

template<typename TKey, typename TValue>
using TMap = std::map<TKey, TValue>;

template<typename TKey, typename TValue>
using TUnorderedMap = std::unordered_map<TKey, TValue>;

template <class T, int L>
constexpr int ArraySize(const T(&arr)[L]) { return L; }

template<class T1, class T2>
using TPair = std::pair<T1, T2>;