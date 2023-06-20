#pragma once
#include "TVector.h"
#include "macro.h"
template <typename T>
struct CRefRange{
	const T* Data;
	const uint32 Size;
	CRefRange(): Data(nullptr), Size(0){}
	CRefRange(const T* data, const uint32 size): Data(data), Size(size){}
	CRefRange(const TVector<T>& vec): Data(vec.Data()), Size(vec.Size()){}
	CRefRange(const TVector<T>& vec, uint32 start, uint32 size) : Data(&vec[start]), Size(size){}

	operator bool() const { return Data && Size; }

	const T* begin() const { return Data; }
	const T* end() const { return Data + Size; }

	const T& operator[](uint32 i)const { ASSERT(i < Size, "Index out of range"); return Data[i]; }
};

template<typename T>
struct RefRange {
	T* Data;
	const uint32 Size;
	RefRange() : Data(nullptr), Size(0) {}
	RefRange(T* data, const uint32 size): Data(data), Size(size){}
	RefRange(TVector<T>& vec): Data(vec.Data), Size(vec.Size()){}
	RefRange(TVector<T>& vec, uint32 start, uint32 size): Data(&vec[start]), Size(size){}

	operator bool() const { return Data && Size; }
	T* begin() { return Data; }
	T* end() { return Data + Size; }
	const T* begin() const { return Data; }
	const T* end()const { return Data + Size; }

	T& operator[](uint32 i) { ASSERT(i < Size, "Index out of range"); return Data[i]; }
	const T& operator[](uint32 i) const { ASSERT(i < Size, "Index out of range"); return Data[i]; }
};
	