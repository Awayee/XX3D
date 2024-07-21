#pragma once
#include <vector>
#include <functional>
#include "Algorithm.h"
#include "Defines.h"
#include "TArrayView.h"

template <class T>
class TVector : private std::vector<T> {
private:
	typedef std::vector<T> Base;
public:
	TVector() : Base() {}

	TVector(uint32 size):Base(size){}
	TVector(uint32 size, const T& val):Base(size, val){}

	TVector(std::initializer_list<T> p) {
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

	uint32 Size() const { return static_cast<uint32>(Base::size()); }

	uint32 ByteSize() const { return static_cast<uint32>(Base::size() * sizeof(T)); }

	void PushBack(T&& ele) { Base::push_back(std::forward<T>(ele)); }

	void PushBack(const T& ele) { Base::push_back(ele); }

	void PushBack(uint32 count, const T* pEle) {
		Base::resize(Base::size() + count);
		void* cpyDst = Base::empty() ? Base::data() : &Base::back();
		memcpy(cpyDst, pEle, count * sizeof(T));
	}

	template<typename ...Args>
	T& EmplaceBack(Args&&...args) { return Base::emplace_back(std::forward<Args>(args)...); }

	void PopBack() { Base::pop_back(); }

	void Clear() { Base::clear(); }

	void Resize(uint32 size) { Base::resize(size); }
	void Resize(uint32 size, const T& val) { Base::resize(size, val); }

	void Reserve(uint32 size) { Base::reserve(size); }

	T* Data() { return Base::data(); }
	const T* Data() const { return Base::data(); }

	T& Back() { return Base::back(); }
	const T& Back() const { return Base::back(); }

	bool Empty() const { return Base::empty(); }

	typedef std::function<bool(const T&, const T&)>  SortFunc;
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

	void Swap(TVector& r) {
		Base::swap(r);
	}

	void Replace(T oldVal, T newVal) {
		for (uint32 i = 0; i < Base::size(); ++i) {
			if (Base::at(i) == oldVal) {
				Base::at(i) = newVal;
			}
		}
	}
};
