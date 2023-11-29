#pragma once
#include "Core/Public/TypeDefine.h"
#include "Core/Public/TArrayView.h"
#include <initializer_list>

// a simple array just contains data
template<typename T, typename SizeType=uint32>
class TFixedArray {
private:
	T* data;
public:
	TFixedArray(SizeType size) {
		data = new T[size];
	}
	~TFixedArray() {
		delete[] data;
	}
	T& operator[](SizeType i) {
		return data[i];
	}
	const T& operator[](SizeType i)const {
		return data[i];
	}

	T* Data() {
		return data;
	}
	const T* Data() const {
		return data;
	}
};




// an array with more info
// can be replaced by TVector
template<typename T, typename SizeType=uint32>
class TArray {
private:
	SizeType size;
	T* data;
#define _TArrayFree if(data) delete[] data
#define _TArrayAllocate data = new T[size]
public:
	TArray() {
		data = nullptr;
		size = 0;
	}
	TArray(const TArray<T>& other) {
		if (this != &other) {
			_TArrayFree;
			size = other.size;
			if (size > 0) {
				_TArrayAllocate;
				memcpy(data, other.data, sizeof(T) * size);
			}
			else {
				data = nullptr;
			}
		}
	}
	TArray(TArray<T>&& other) {
		data = other.data;
		size = other.size;
		other.data = nullptr;
		other.size = 0u;
	}

	TArray(SizeType l) {
		size = l;
		_TArrayAllocate;
	}

	TArray(std::initializer_list<T> p) {
		size = p.size();
		_TArrayAllocate;
		auto begin = p.begin();
		for (SizeType i = 0; i < size; ++i) {
			data[i] = *(begin + i);
		}
	}
	TArray(T* other, SizeType l) {
		size = l;
		if (size > 0) {
			_TArrayAllocate;
			memcpy(data, other, sizeof(T) * size);
		}
		else {
			data = nullptr;
		}
	}
	~TArray() {
		Clear();
	}

	SizeType Size()const {
		return size;
	}

	T* Data()const {
		return data;
	}

	void Clear() {
		if (data != nullptr) {
			_TArrayFree;
		}
		size = 0;
	}

	void Swap(TArray<T>& other) {
		T* tempData = data;
		SizeType tempSize = size;
		data = other.data;
		size = other.size;
		other.data = tempData;
		other.size = size;
	}

	T* begin()const {
		return data;
	}

	T* end()const {
		return data + size;
	}

	T& operator[](SizeType i) {
		return data[i];
	}

	const T& operator[](SizeType i)const {
		return data[i];
	}

	void Resize(SizeType l) {
		if (size != l) {
			_TArrayFree;

			size = l;
			if (l == 0) {
				data = nullptr;
			}
			else {
				_TArrayAllocate;

			}
		}
	}

	TArray& operator=(const TArray& other) {
		if (this != &other) {
			_TArrayFree;
			size = other.size;
			if (size > 0) {
				_TArrayAllocate;
				memcpy(data, other.data, sizeof(T) * size);
			}
			else {
				data = nullptr;
			}
		}
		return *this;
	}
};

template <typename T, uint32 L>
class TStaticArray{
public:
	~TStaticArray() = default;
	TStaticArray() = default;
	TStaticArray(const TStaticArray& rhs) {
		memcpy(m_Data, rhs.m_Data, ByteSize());
	}
	TStaticArray(TStaticArray&& rhs) noexcept {
		memcpy(m_Data, rhs.m_Data, ByteSize());
	}
	TStaticArray(std::initializer_list<T> params) {
		static_assert(L == params.size());
		memcpy(m_Data, params.begin(), ByteSize());
	}

	TStaticArray& operator =(const TStaticArray& rhs) {
		if(this != &rhs) {
			memcpy(m_Data, rhs.m_Data, ByteSize());
		}
		return *this;
	}

	TStaticArray& operator=(TStaticArray&& rhs) noexcept {
		memcpy(m_Data, rhs.m_Data, ByteSize());
		return *this;
	}

	operator TArrayView<T>() {
		return TArrayView<T>{m_Data, L};
	}

	operator TArrayView<T>() const {
		return TConstArrayView<T>{m_Data, L};
	}

	constexpr uint32 Size() const {
		return L;
	}

	constexpr uint32 ByteSize() const {
		return L * sizeof(T);
	}

	T& operator[](uint32 i) {
		return m_Data[i];
	}

	const T& operator[](uint32 i)const {
		return m_Data[i];
	}

	bool operator == (const TStaticArray& rhs) const {
		if(m_Data == rhs.m_Data) {
			return true;
		}
		return 0 ==memcmp(m_Data, rhs.m_Data, L);
	}

	// for-each loop
	T* begin()const {
		return m_Data;
	}

	T* end()const {
		return m_Data + L;
	}

private:
	T m_Data[L];
};