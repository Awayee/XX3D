#pragma once
#include <unordered_map>
#include <map>
#include "TArray.h"
#include <string>
#include <unordered_set>
#include <set>
#include "Defines.h"
#include "TList.h"


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
//using TArray =  std::vector<T>;

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

// a link-list for data reusing
class FreeListAllocator {
public:
	static constexpr uint32 INVALID = UINT32_MAX;
	NON_COPYABLE(FreeListAllocator);
	struct Range { uint32 Start, End; };
	FreeListAllocator() = default;
	FreeListAllocator(FreeListAllocator&& rhs) noexcept;
	FreeListAllocator& operator=(FreeListAllocator&& rhs) noexcept;
	// allocate data, return start index, if failed, return INVALID
	uint32 Allocate(uint32 allocSize);
	// free allocated data, by start index and size
	void Free(uint32 allocStart, uint32 allocSize);

	bool IsEmpty() const;
private:
	TDoubleLinkList<Range> m_Ranges;
};