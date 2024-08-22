#pragma once
#include "Log.h"

template<typename T, typename SizeType=uint32>
class TArrayView {
private:
	T* m_Data;
	SizeType m_Size;
public:
	TArrayView() : m_Data(nullptr), m_Size(0) {}
	TArrayView(const TArrayView& rhs): m_Data(rhs.m_Data), m_Size(rhs.m_Size){}
	TArrayView(TArrayView&& rhs) noexcept: m_Data(rhs.m_Data), m_Size(rhs.m_Size) {
		rhs.m_Data = nullptr;
		rhs.m_Size = 0;
	}
	TArrayView(T* data, const SizeType size): m_Data(data), m_Size(size){}
	TArrayView(T& data) : m_Data(&data), m_Size(1){}
	TArrayView& operator=(const TArrayView& rhs) {
		if(this == &rhs) {
			return *this;
		}
		m_Data = rhs.m_Data;
		m_Size = rhs.m_Size;
		return *this;
	}
	TArrayView& operator=(TArrayView&& rhs) noexcept {
		m_Data = rhs.m_Data;
		m_Size = rhs.m_Size;
		rhs.m_Data = nullptr;
		rhs.m_Size = 0;
		return *this;
	}

	bool operator==(const TArrayView& rhs) {
		return m_Data == rhs.m_Data && m_Size == rhs.m_Size;
	}

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
	SizeType ByteSize() const { return m_Size * sizeof(T); }
};

template<typename T, typename SizeType=uint32> using TConstArrayView = TArrayView<const T, SizeType>;
	