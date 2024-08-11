#pragma once
#include "Defines.h"

template<class T>
class TUniquePtr {
public:
	NON_COPYABLE(TUniquePtr);
	TUniquePtr(): m_Ptr(nullptr) {}
	explicit TUniquePtr(T* ptr) {
		m_Ptr = ptr;
	}

	template<class Ty> explicit TUniquePtr(Ty* ptr) {
		m_Ptr = static_cast<T*>(ptr);
	}

	TUniquePtr(decltype(nullptr)) {}

	~TUniquePtr() {
		Free();
	}

	template<class Ty> TUniquePtr(const TUniquePtr<Ty>&) = delete;

	template<class Ty> TUniquePtr& operator=(const TUniquePtr<Ty>& rhs) = delete;

	TUniquePtr(TUniquePtr&& rhs) noexcept {
		m_Ptr = rhs.Release();
	}

	TUniquePtr& operator=(TUniquePtr&& rhs) noexcept {
		Free();
		m_Ptr = rhs.Release();
		return *this;
	}

	template<class Ty> TUniquePtr(TUniquePtr<Ty>&& rhs) noexcept {
		m_Ptr = static_cast<T*>(rhs.Release());
	}

	template<class Ty> TUniquePtr& operator=(TUniquePtr<Ty>&& rhs) noexcept {
		Free();
		m_Ptr = static_cast<T*>(rhs.Release());
		return *this;
	}

	operator T*(){
		return m_Ptr;
	}

	explicit operator bool () const {
		return !!m_Ptr;
	}

	T* operator->() {
		return m_Ptr;
	}

	const T* operator->() const {
		return m_Ptr;
	}

	T& operator* () {
		return *m_Ptr;
	}

	const T& operator*() const {
		return *m_Ptr;
	}

	template<class Ty>
	bool operator == (Ty* rhs) const {
		return m_Ptr == rhs;
	}

	T* Get() {
		return m_Ptr;
	}

	const T* Get() const {
		return m_Ptr;
	}

	template<class ...Args>
	static TUniquePtr<T> New(Args&& ...args) {
		return TUniquePtr<T>(new T(args...));
	}

	void Reset() {
		if (nullptr != m_Ptr) {
			Free();
		}
	}

	void Reset(T* ptr) {
		Reset();
		m_Ptr = ptr;
	}

	template <class Ty> void Reset(Ty * ptr) {
		Reset((T*)ptr);
	}

	T* Release() {
		T* ptr = m_Ptr;
		m_Ptr = nullptr;
		return ptr;
	}
	
private:
	T* m_Ptr{nullptr};

	inline void Free() {
		if(m_Ptr) {
			delete m_Ptr;
			m_Ptr = nullptr;
		}
	}
};