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
	template <class Ty> TUniquePtr(const TUniquePtr<Ty>&) = delete;
	template <class Ty> TUniquePtr(TUniquePtr<Ty>&& rhs) {
		m_Ptr = (T*)rhs.m_Ptr;
		rhs.m_Ptr = nullptr;
	}
	template <class Ty> TUniquePtr(Ty* ptr) {
		m_Ptr = (T*)ptr;
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
		delete m_Ptr;
		m_Ptr = nullptr;
	}
};