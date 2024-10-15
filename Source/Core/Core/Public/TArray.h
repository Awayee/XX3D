#pragma once
#include "Core/Public/Defines.h"
#include "Core/Public/TArrayView.h"
#include "Core/Public/Func.h"
#include <initializer_list>
#include <vector>

// a simple array just contains data
template<typename T, typename SizeType=uint32>
class TFixedArray {
private:
	T* data;
public:
	TFixedArray(SizeType size) {
		data = size > 0 ? (new T[size]) : nullptr;
	}
	TFixedArray(const TFixedArray&) = delete;
	TFixedArray(TFixedArray&& rhs)noexcept {
		data = rhs.data;
		rhs.data = nullptr;
	}
	TFixedArray& operator=(const TFixedArray&) = delete;
	TFixedArray& operator=(TFixedArray&& rhs) noexcept{
		data = rhs.data;
		rhs.data = nullptr;
		return *this;
	}
	~TFixedArray() {
		if(data) {
			delete[] data;
		}
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

/*
// an array with more info
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
*/

template <typename T, uint32 L>
class TStaticArray{
public:
	~TStaticArray() = default;
	TStaticArray() = default;
	TStaticArray(const TStaticArray& rhs) {
		memcpy(m_Data, rhs.m_Data, ByteSize());
	}
	TStaticArray(TStaticArray&& rhs) noexcept = delete;

	TStaticArray(std::initializer_list<T> params) {
		memcpy(m_Data, params.begin(), params.size() * sizeof(T));
	}
	TStaticArray(const T& fillValue) {
		Reset(fillValue);
	}

	TStaticArray& operator=(const TStaticArray& rhs) {
		if(this != &rhs) {
			memcpy(m_Data, rhs.m_Data, ByteSize());
		}
		return *this;
	}

	TStaticArray& operator=(TStaticArray&& rhs) noexcept = delete;

	TStaticArray& operator=(std::initializer_list<T> params) {
		memcpy(m_Data, params.begin(), params.size() * sizeof(T));
		return *this;
	}

	operator TArrayView<T>() {
		return TArrayView<T>{m_Data, L};
	}

	operator TConstArrayView<T>() const {
		return TConstArrayView<T>{m_Data, L};
	}

	constexpr uint32 Size() const {
		return L;
	}

	constexpr uint32 ByteSize() const {
		return L * sizeof(T);
	}

	T* Data() {
		return m_Data;
	}

	const T* Data() const {
		return m_Data;
	}

	void Reset(const T& fillValue) {
		for (uint32 i = 0; i < L; ++i) {
			m_Data[i] = fillValue;
		}
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
	const T* begin()const {
		return m_Data;
	}
	const T* end()const {
		return m_Data + L;
	}
	T* begin() {
		return m_Data;
	}
	T* end() {
		return m_Data + L;
	}

private:
	T m_Data[L];
};

template<uint32 L>
class TStaticArray<bool, L> {
	static constexpr uint32 BYTE = 8;
	static constexpr uint32 BYTE_SIZE = (L + BYTE - 1) / BYTE;
	static constexpr uint8 BYTE_BIT_MAX = 1 << 7;
public:
	struct BitReference {
		uint8* BytePtr;
		const uint8 Mask;
		operator bool() const {
			return !!((*BytePtr) & Mask);
		}
		BitReference& operator=(bool val) {
			*BytePtr = val ? ((*BytePtr) | Mask) : ((*BytePtr) & (~Mask));
			return *this;
		}
		BitReference& operator=(const BitReference& rhs) {
			if(this == &rhs) {
				return *this;
			}
			return operator=((bool)rhs);
		}
		bool operator==(const BitReference& rhs) const {
			return (bool)*this == (bool)rhs;
		}
	};
	struct BitIterator {
		uint8* BytePtr;
		uint8  Mask;
		BitReference operator*() {
			return BitReference{ BytePtr, Mask };
		}
		void operator++() {
			if(Mask == BYTE_BIT_MAX) {
				++BytePtr;
				Mask = 1;
			}
			else {
				Mask <<= 1;
			}
		}
		bool operator==(const BitIterator& rhs) const {
			return Mask == rhs.Mask && BytePtr == rhs.BytePtr;
		}
	};
	struct ConstBitIterator {
		const uint8* BytePtr;
		uint8 Mask;
		bool operator*() const {
			return !!((*BytePtr) & Mask);
		}
		void operator++() {
			if (Mask == BYTE_BIT_MAX) {
				++BytePtr;
				Mask = 1;
			}
			else {
				Mask <<= 1;
			}
		}
		bool operator==(const ConstBitIterator& rhs) const {
			return Mask == rhs.Mask && BytePtr == rhs.BytePtr;
		}
	};

	NON_MOVEABLE(TStaticArray);
	~TStaticArray() = default;
	TStaticArray() = default;
	TStaticArray(const TStaticArray& rhs) {
		memcpy(m_Data, rhs.m_Data, ByteSize());
	}

	TStaticArray(bool fillValue) {
		Reset(fillValue);
	}

	TStaticArray& operator=(const TStaticArray& rhs) {
		if (this != &rhs) {
			memcpy(m_Data, rhs.m_Data, ByteSize());
		}
		return *this;
	}

	constexpr uint32 Size() const {
		return L;
	}

	constexpr uint32 ByteSize() const {
		return BYTE_SIZE;
	}

	void Reset(bool fillValue) {
		uint8 val = (uint8)fillValue;
		for(uint32 i=0; i<BYTE_SIZE; ++i) {
			m_Data[i] = val;
		}
	}

	BitReference operator[](uint32 i) {
		CHECK(i < Size());
		const uint32 byteIndex = i / BYTE;
		const uint32 byteOffset = i % BYTE;
		return BitReference{ &m_Data[byteIndex], 1u << byteOffset };
	}

	bool operator[](uint32 i) const {
		const uint32 byteIndex = i / BYTE;
		const uint32 byteOffset = i % BYTE;
		return m_Data[byteIndex] & (1 << byteOffset);
	}

	bool operator == (const TStaticArray& rhs) const {
		if (m_Data == rhs.m_Data) {
			return true;
		}
		return 0 == memcmp(m_Data, rhs.m_Data, L);
	}

	// for-each loop
	ConstBitIterator begin()const {
		return ConstBitIterator{m_Data, 1};
	}
	ConstBitIterator end()const {
		return ConstBitIterator{ &m_Data[BYTE_SIZE - 1], BYTE_BIT_MAX };
	}
	BitIterator begin() {
		return BitIterator{ m_Data, 1 };
	}
	BitIterator end() {
		return BitIterator{ &m_Data[BYTE_SIZE - 1], BYTE_BIT_MAX };
	}

private:
	uint8 m_Data[BYTE_SIZE];

	void Set(uint32 i, bool val) {
		CHECK(i < Size());
		const uint32 byteIndex = i / BYTE;
		const uint32 byteOffset = i % BYTE;
		uint8 byteMask = 1 << byteOffset;
		if(val) {
			m_Data[byteIndex] |= byteMask;
		}
		else {
			m_Data[byteIndex] &= (~byteMask);
		}
	}

	bool Get(uint32 i) {
		CHECK(i < Size());
		const uint32 byteIndex = i / BYTE;
		const uint32 byteOffset = i % BYTE;
		uint8 byteMask = 1 << byteOffset;
		return m_Data[byteIndex] & byteMask;
	}
};

template<uint32 L> using TStaticBitArray = TStaticArray<bool, L>;

// A dynamic array derived from std::vector.
template <class T>
class TArray : private std::vector<T> {
private:
	typedef std::vector<T> Base;
public:
	TArray() : Base() {}

	TArray(uint32 size) :Base(size) {}
	TArray(uint32 size, const T& val) :Base(size, val) {}

	TArray(std::initializer_list<T> p) {
		Base::resize(p.size());
		auto begin = p.begin();
		for (unsigned int i = 0; i < p.size(); i++) {
			Base::at(i) = *(begin + i);
		}
	}

	using Base::operator[];
	using Base::operator=;
	using Base::begin;
	using Base::end;

	operator TArrayView<T>() {
		return TArrayView<T>(Data(), Size());
	}

	operator TConstArrayView<T>() const {
		return TConstArrayView<T>(Data(), Size());
	}

	uint32 Size() const { return static_cast<uint32>(Base::size()); }

	uint32 ByteSize() const { return static_cast<uint32>(Base::size() * sizeof(T)); }

	void PushBack(T&& ele) { Base::push_back(std::forward<T>(ele)); }

	void PushBack(const T& ele) { Base::push_back(ele); }

	void PushBack(uint32 count, const T* pEle) {
		Base::resize(Base::size() + count);
		void* cpyDst = Base::empty() ? Base::data() : &Base::back();
		memcpy(cpyDst, pEle, count * sizeof(T));
	}

	void PushBack(const TArray& rhs) {
		Base::insert(Base::end(), rhs.begin(), rhs.end());
	}

	template<typename ...Args>
	T& EmplaceBack(Args&&...args) { return Base::emplace_back(std::forward<Args>(args)...); }

	void PopBack() { Base::pop_back(); }

	void Resize(uint32 size) { Base::resize(size); }
	void Resize(uint32 size, const T& val) { Base::resize(size, val); }

	void Reserve(uint32 size) { Base::reserve(size); }

	T* Data() { return Base::data(); }
	const T* Data() const { return Base::data(); }

	T& Back() { return Base::back(); }
	const T& Back() const { return Base::back(); }

	bool IsEmpty() const { return Base::empty(); }

	void Reset() { Base::clear(); }

	// Clear and reallocate memory.
	void Empty(uint32 size) {
		Base::swap(TArray{});
		Resize(size);
	}

	typedef std::function<bool(const T&, const T&)>  SortFunc;
	void Sort(uint32 start, uint32 end, const SortFunc& f) {
		std::sort(Base::begin() + start, Base::begin() + end, f);
	}
	void Sort(const SortFunc& f) { std::sort(Base::begin(), Base::end(), f); }

	void Sort() { std::sort(Base::begin(), Base::end(), std::less<T>()); }

	void RemoveAt(uint32 i) {
		Base::erase(Base::begin() + i);
	}

	void SwapRemove(const T& ele) {
		for (uint32 i = 0; i < Base::size(); ++i) {
			if (Base::at(i) == ele) {
				std::swap(Base::at(i), Base::back());
				Base::pop_back();
				break;
			}
		}
	}

	void SwapRemoveAt(uint32 i) {
		std::swap(Base::at(i), Base::back());
		Base::pop_back();
	}

	void Swap(TArray& r) {
		Base::swap(r);
	}

	void SwapEle(uint32 i, uint32 j) {
		std::swap(Base::at(i), Base::at(j));
	}

	void Replace(T oldVal, T newVal) {
		for (uint32 i = 0; i < Base::size(); ++i) {
			if (Base::at(i) == oldVal) {
				Base::at(i) = newVal;
			}
		}
	}
};