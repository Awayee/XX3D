#pragma once

#include <initializer_list>

// a simple array just contains data
template<typename T>
class TempArray {
private:
	T* data;
public:
	TempArray(unsigned int size) {
		data = new T[size];
	}
	~TempArray() {
		delete[] data;
	}
	T& operator[](unsigned int i) {
		return data[i];
	}
	const T& operator[](unsigned int i)const {
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
template<typename T>
class TArray {
private:
	unsigned int size;
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

	TArray(unsigned int l) {
		size = l;
		_TArrayAllocate;
	}

	TArray(std::initializer_list<T> p) {
		size = p.size();
		_TArrayAllocate;
		auto begin = p.begin();
		for (unsigned int i = 0; i < size; i++) {
			data[i] = *(begin + i);
		}
	}
	TArray(T* other, unsigned int l) {
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

	unsigned int Size()const {
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
		unsigned int tempSize = size;
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

	T& operator[](int i) {
		return data[i];
	}

	const T& operator[](int i)const {
		return data[i];
	}

	void Resize(unsigned int l) {
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

	TArray<T>& operator=(const TArray<T>& other) {
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