#pragma once
#include "TVector.h"
#include "macro.h"
template <typename T, typename SizeType=uint32>
class CRefRange{
private:
	const T* m_Data;
	const SizeType m_Size;
public:
	CRefRange(): m_Data(nullptr), m_Size(0){}
	CRefRange(const T* data, const SizeType size): m_Data(data), m_Size(size){}
	CRefRange(const TVector<T>& vec): m_Data(vec.Data()), m_Size(vec.Size()){}
	CRefRange(const TVector<T>& vec, SizeType start, SizeType size) : m_Data(&vec[start]), m_Size(size){}

	operator bool() const { return m_Data && m_Size; }

	const T* begin() const { return m_Data; }
	const T* end() const { return m_Data + m_Size; }

	const T& operator[](SizeType i)const { ASSERT(i < m_Size, "Index out of range"); return m_Data[i]; }

	SizeType Size()const { return m_Size; }
	const T* Data()const { return m_Data; }
};

template<typename T, typename SizeType=uint32>
class RefRange {
private:
	T* m_Data;
	const SizeType m_Size;
	RefRange() : m_Data(nullptr), m_Size(0) {}
	RefRange(T* data, const SizeType size): m_Data(data), m_Size(size){}
	RefRange(TVector<T>& vec): m_Data(vec.m_Data), m_Size(vec.m_Size()){}
	RefRange(TVector<T>& vec, SizeType start, SizeType size): m_Data(&vec[start]), m_Size(size){}

	operator bool() const { return m_Data && m_Size; }
	T* begin() { return m_Data; }
	T* end() { return m_Data + m_Size; }
	const T* begin() const { return m_Data; }
	const T* end()const { return m_Data + m_Size; }

	T& operator[](SizeType i) { ASSERT(i < m_Size, "Index out of range"); return m_Data[i]; }
	const T& operator[](SizeType i) const { ASSERT(i < m_Size, "Index out of range"); return m_Data[i]; }

	T* Data() { return m_Data; }
	const T* Data() const { return m_Data; }
	SizeType Size() const { return m_Size; }

};
	