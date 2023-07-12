#pragma once
#include "Algorithm.h"
#include <memory>

template <class T, class Dx = std::default_delete<T>>
using TUniquePtr = std::unique_ptr<T, Dx>;

template <class T, class ...Args>
inline TUniquePtr<T> MakeUniquePtr(Args...args) {
	return std::make_unique<T>(args...);
}

template <class T>
using TSharedPtr = std::shared_ptr<T>;

template <class T, class ...Args>
inline TSharedPtr<T> MakeSharedPtr(Args...args) {
	return std::make_shared<T>(args...);
}

template <class T>
using TWeakPtr = std::weak_ptr<T>;

/*
//unique ptr
template <class T, class Dx=std::default_delete<T>>
class TUniquePtr : private std::unique_ptr<T, Dx> {
private:
	using Base = std::unique_ptr<T, Dx>;
public:
	
	TUniquePtr() : Base() {}
	TUniquePtr(T* p): Base(p){}
	template<class T2> TUniquePtr(T2* p): Base(p){}
	TUniquePtr(const TUniquePtr& p) = delete;
	TUniquePtr(TUniquePtr&& p) noexcept: Base(std::forward<TUniquePtr>(p)){}
	template<class T2, class Dx2> TUniquePtr(TUniquePtr<T2, Dx2>&& p) noexcept: Base(p.Release()){}
	
	using Base::operator*;
	using Base::operator->;
	using Base::operator=;
	using Base::operator bool;

	void Reset(T* p) { Base::reset(p); }
	void Reset() { Base::reset(); }

	T* Release() { return Base::release(); }
	void Swap(TUniquePtr& p) { Base::swap(p); }

	T* Get() { return Base::get(); }
	_NODISCARD const T* Get()const { return Base::get(); }
};


template <class T1, class Dx1, class T2, class Dx2>
_NODISCARD bool operator==(const TUniquePtr<T1, Dx1>& p1, const TUniquePtr<T2, Dx2>& p2) noexcept {
	return p1.Get() == p2.Get();
}

template <class T1, class T2>
_NODISCARD bool operator!=(const TUniquePtr<T1>& p1, const TUniquePtr<T2>& p2) noexcept {
	return p1.Get() != p2.Get();
}

template<class T, class Dx, class T1>
_NODISCARD bool operator==(const TUniquePtr<T, Dx>& p1, T1 p2) noexcept {
	return p1.Get() == p2;
}

template<class T, class Dx, class T1>
_NODISCARD bool operator!=(const TUniquePtr<T, Dx>& p1, T1 p2) noexcept {
	return p1.Get() != p2;
}

template<class T, class Dx, class T1>
_NODISCARD bool operator==(T1 p2, const TUniquePtr<T, Dx>& p1) noexcept {
	return p1.Get() == p2;
}

template<class T, class Dx, class T1>
_NODISCARD bool operator!=(T1 p2, const TUniquePtr<T, Dx>& p1) noexcept {
	return p1.Get() != p2;
}

template <class T, class ...Args>
TUniquePtr<T> MakeUniquePtr(Args&&...args) {
	return TUniquePtr<T>(new T(std::forward<Args>(args)...));
}


//shared ptr
template <class T>
class TSharedPtr : private std::shared_ptr<T> {
private:
	using Base = std::shared_ptr<T>;
public:
	TSharedPtr(): Base(){}
	TSharedPtr(const TSharedPtr<T>& p) : Base(p) {}
	TSharedPtr(TSharedPtr<T>&& p) noexcept : Base(std::forward<TSharedPtr<T>>(p)) {}
	template<class T2> TSharedPtr(const TSharedPtr<T2>& p) : Base(p) {}
	template<class T2> TSharedPtr(TSharedPtr<T2>&& p) : Base(std::forward<TSharedPtr<T2>>(p)) {}

	using Base::operator=;
	using Base::operator*;
	using Base::operator->;
	using Base::operator bool;

	void Reset(T* p) { Base::reset(p); }
	void Reset() { Base::reset(); }

	T* Get() { return Base::get(); }
	const T* Get() const { return Base::get(); }
};

template <class T1, class T2>
_NODISCARD bool operator==(const TSharedPtr<T1>& p1, const TSharedPtr<T2>& p2) noexcept {
	return p1.Get() == p2.Get();
}

template <class T1, class T2>
_NODISCARD bool operator!=(const TSharedPtr<T1>& p1, const TSharedPtr<T2>& p2) noexcept {
	return p1.Get() != p2.Get();
}

template <class T, class ...Args>
inline TSharedPtr<T> MakeSharedPtr(Args...args) {
	return TSharedPtr<T>(args...);
}


//weak ptr
template <class T>
class TWeakPtr: private std::weak_ptr<T> {
private:
	using Base = std::weak_ptr<T>;
public:
	TWeakPtr() : Base() {}
	TWeakPtr(const TWeakPtr<T>& p): Base(p){}
	TWeakPtr(TWeakPtr<T>&& p) noexcept :Base(p) {}
	template <class T2> TWeakPtr(const TWeakPtr<T2>& p) : Base(p) {}
	template <class T2> TWeakPtr(TWeakPtr<T2>&& p) : Base(std::forward<TWeakPtr<T2>>(p)) {}
	TWeakPtr(const TSharedPtr<T>& p) : Base(p) {}
	TWeakPtr(TSharedPtr<T>&& p): Base(p){}
	template <class T2> TWeakPtr(const TSharedPtr<T2>& p):Base(p){}
	template <class T2> TWeakPtr(TSharedPtr<T2>&& p) : Base(std::forward<TSharedPtr<T2>>(p)) {}

	using Base::operator=;

	void Reset() { Base::reset(); }
	void Swap(TWeakPtr& p) { Base::swap(p); }
	_NODISCARD bool IsValid() const { return Base::expired(); }
	_NODISCARD TSharedPtr<T> Pin() { return Base::lock(); }
};

//overloads of Swap function
template <class T, class Dx> inline void Swap(TUniquePtr<T, Dx>& l, TUniquePtr<T, Dx>& r) noexcept {
	l.Swap(r);
}

template <class T> inline void Swap(TSharedPtr<T>& l, TSharedPtr<T>& r) noexcept {
	l.Swap(r);
}

template <class T> inline void Swap(TWeakPtr<T>& l, TWeakPtr<T>& r) noexcept {
	l.Swap(r);
}
*/
