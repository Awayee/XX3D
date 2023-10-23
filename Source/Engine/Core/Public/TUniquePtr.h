#pragma once

template<class T>
class TUniquePtr {
public:
	~TUniquePtr() {
		_Free();
	}
	TUniquePtr() = default;
	TUniquePtr(T* ptr) {
		m_Ptr = ptr;
	}
	template<class Ty> TUniquePtr(const TUniquePtr<Ty>&) = delete;
	template<class Ty> TUniquePtr(TUniquePtr<Ty>&& rhs) {
		m_Ptr = static_cast<T*>(rhs.m_Ptr);
	}
	template<class Ty> explicit TUniquePtr(Ty* ptr) {
		m_Ptr = static_cast<T*>(ptr);
	}

	template<class Ty> TUniquePtr& operator=(const TUniquePtr<Ty>& rhs) = delete;
	template<class Ty> TUniquePtr& operator=(TUniquePtr<Ty>&& rhs) {
		m_Ptr = static_cast<T*>(rhs.m_Ptr);
		return *this;
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

	void Reset(T* ptr) {
		if(nullptr != m_Ptr) {
			_Free();
		}
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

	inline void _Free() {
		if(m_Ptr) {
			delete m_Ptr;
			m_Ptr = nullptr;
		}
	}
};