#pragma once

#include <initializer_list>

// a simple array just contains data
template<typename T>
class TArray {
private:
	T* data;
public:
	TArray(unsigned int size) {
		data = new T[size];
	}
	~TArray() {
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



#define _TArrayFree if(data) delete[] data
#define _TArrayAllocate data = new T[size]

// an array with more info
// can be replaced by TVector
template<typename T>
class TArray2 {
private:
	unsigned int size;
	T* data;
public:
	TArray2() {
		data = nullptr;
		size = 0;
	}
	TArray2(const TArray2<T>& other) {
		if (this != &other) {
			_TArrayFree();
			size = other.size;
			if (size > 0) {
				_TArrayAllocate();
				memcpy(data, other.data, sizeof(T) * size);
			}
			else {
				data = nullptr;
			}
		}
	}
	TArray2(TArray2<T>&& other) {
		data = other.data;
		size = other.size;
		other.data = nullptr;
		other.size = 0u;
	}

	TArray2(unsigned int l) {
		size = l;
		_TArrayAllocate();
	}

	TArray2(std::initializer_list<T> p) {
		size = p.size();
		_TArrayAllocate();
		auto begin = p.begin();
		for (unsigned int i = 0; i < size; i++) {
			data[i] = *(begin + i);
		}
	}
	TArray2(T* other, unsigned int l) {
		size = l;
		if (size > 0) {
			_TArrayAllocate();
			memcpy(data, other, sizeof(T) * size);
		}
		else {
			data = nullptr;
		}
	}
	~TArray2() {
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
			_TArrayFree();
		}
		size = 0;
	}

	void Swap(TArray2<T>& other) {
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
			_TArrayFree();

			size = l;
			if (l == 0) {
				data = nullptr;
			}
			else {
				_TArrayAllocate();

			}
		}
	}

	TArray2<T>& operator=(const TArray2<T>& other) {
		if (this != &other) {
			_TArrayFree();
			size = other.size;
			if (size > 0) {
				_TArrayAllocate();
				memcpy(data, other.data, sizeof(T) * size);
			}
			else {
				data = nullptr;
			}
		}
		return *this;
	}
};