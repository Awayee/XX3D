#pragma once
#include <unordered_map>
#include <map>
#include "Core/Public/TArray.h"
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


// an array with reusable ids, type T must has default constructor.
template<class T> struct DefaultDestructor {
	void operator()(T& t) {}
};
template<class T, class Destructor = DefaultDestructor<T>>
class TIDReuseArray {
public:
	static constexpr uint32 INVALID = FreeListAllocator::INVALID;
	NON_COPYABLE(TIDReuseArray);
	TIDReuseArray() = default;
	TIDReuseArray(TIDReuseArray&& rhs) noexcept : m_Data(MoveTemp(rhs.m_Data)), m_FreeIDs(MoveTemp(rhs.m_FreeIDs)){}
	TIDReuseArray& operator=(TIDReuseArray&& rhs) noexcept {
		m_Data = MoveTemp(rhs.m_Data);
		m_FreeIDs = MoveTemp(rhs.m_FreeIDs);
		return *this;
	}
	uint32 Allocate() {
		uint32 idx = m_FreeIDs.Allocate(1);
		if(INVALID == idx) {
			idx = m_Data.Size();
			m_Data.EmplaceBack();
		}
		if(m_FreeIDs.IsEmpty()) {
			idx = m_Data.Size();
			m_Data.EmplaceBack();
		}
		return idx;
	}
	void Free(uint32 idx) {
		Destructor{}(m_Data[idx]);
		m_FreeIDs.Free(idx, 1);
	}
	T& Get(uint32 idx) {
		return m_Data[idx];
	}
	const T& Get(uint32 idx) const {
		return m_Data[idx];
	}

	// size with unused.
	uint32 Size() {
		return m_Data.Size();
	}

private:
	TArray<T> m_Data;
	FreeListAllocator m_FreeIDs;
};


template<class T>
class TIDMapContainer {
public:
	NON_COPYABLE(TIDMapContainer);
	TIDMapContainer() = default;
	TIDMapContainer(TIDMapContainer&& rhs) noexcept: m_Array(rhs.m_Array), m_IDMap(rhs.m_IDMap) {}
	TIDMapContainer& operator=(TIDMapContainer&& rhs) noexcept {
		m_Array = MoveTemp(rhs.m_Array);
		m_IDMap = MoveTemp(rhs.m_IDMap);
		return *this;
	}

	uint32 FindIdx(uint32 ID) {
		if(auto iter = m_IDMap.find(ID); iter != m_IDMap.end()) {
			return iter->second;
		}
		return INVALID_INDEX;
	}

	T& operator[](uint32 idx) {
		return m_Array[idx];
	}

	const T& operator[](uint32 idx) const {
		return m_Array[idx];
	}

	uint32 FindOrAddID(uint32 ID) {
		if(const uint32 idx = FindIdx(ID); idx != INVALID_INDEX) {
			return idx;
		}
		const uint32 idx = m_Array.Size();
		m_Array.EmplaceBack();
		m_IDMap[ID] = idx;
		return idx;
	}

	uint32 RemoveID(uint32 ID) {
		if(const uint32 idx = FindIdx(ID); idx != INVALID_INDEX) {
			m_Array.SwapRemoveAt(idx);
			return idx;
		}
		return INVALID_INDEX;
	}

	uint32 Size() {
		return m_Array.Size();
	}

	// for-each loop
	T* begin() {
		return m_Array.Data();
	}

	T* end() {
		return m_Array.Data() + Size();
	}

private:
	TArray<T> m_Array;
	TMap<uint32, uint32> m_IDMap;
};