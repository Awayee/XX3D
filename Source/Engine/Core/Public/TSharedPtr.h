#pragma once
#include "Core/Public/RefCounter.h"

template<class T, bool ThreadSafe>
class TWeakPtr;

template <class T, bool ThreadSafe=false>
class TSharedPtr {
	using RefCounterType = TRawRefCounter<ThreadSafe>;
public:
	~TSharedPtr() {
		DecreaseRef();
	}

	TSharedPtr() = default;

	explicit TSharedPtr(T* ptr=nullptr): m_Ptr(ptr), m_Counter(new RefCounterType()) {}

	TSharedPtr(const TSharedPtr& rhs): m_Ptr(rhs.m_Ptr), m_Counter(rhs.m_Counter) {
		m_Counter->IncreaseSharedRef();
	}

	template<class Ty>
	TSharedPtr(const TSharedPtr<Ty, ThreadSafe>& rhs): m_Ptr((T*)(rhs.m_Ptr)), m_Counter(rhs.m_Counter) {
		m_Counter->IncreaseSharedRef();
	}

	TSharedPtr(TSharedPtr&& rhs)noexcept: m_Ptr(rhs.m_Ptr), m_Counter(rhs.m_Counter){
		rhs.m_Ptr = nullptr;
		rhs.m_Counter = nullptr;
	}

	template<class Ty>
	TSharedPtr(TSharedPtr<Ty, ThreadSafe>&& rhs) noexcept: m_Ptr((T*)(rhs.m_Ptr)), m_Counter(rhs.m_Counter) {
		rhs.m_Ptr = nullptr;
		rhs.m_Counter = nullptr;
	}

	TSharedPtr& operator=(const TSharedPtr& rhs) {
		if(this != &rhs) {
			DecreaseRef();
			m_Ptr = rhs.m_Ptr;
			m_Counter = rhs.m_Counter;
		}
		return *this;
	}

	template<class Ty>
	TSharedPtr& operator=(const TSharedPtr<Ty, ThreadSafe>& rhs) {
		if(this != &rhs){
			DecreaseRef();
			m_Counter = rhs.m_Counter;
			m_Ptr = rhs.m_Ptr;
			m_Counter->IncreaseSharedRef();
		}
		return *this;
		
	}

	TSharedPtr& operator=(TSharedPtr&& rhs)noexcept {
		DecreaseRef();
		m_Counter = std::move(rhs.m_Counter);
		m_Ptr = rhs.m_Ptr;
		return this;
		
	}

	template<class Ty>
	TSharedPtr& operator=(TSharedPtr<Ty, ThreadSafe>&& rhs)noexcept {
		DecreaseRef();
		m_Counter = rhs.m_Counter;
		m_Ptr = rhs.m_Ptr;
		return this;
	}

	explicit operator bool() const {
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

	template<class Ty> bool operator == (Ty* rhs) const {
		return m_Ptr == rhs;
	}

	T* Get() {
		return m_Ptr;
	}

	template<class Ty> void Reset(Ty* ptr) {
		DecreaseRef();
		m_Counter = new RefCounterType();
		m_Ptr = ptr;
	}

	void Reset() {
		DecreaseRef();
		m_Counter = nullptr;
		m_Ptr = nullptr;
	}

private:
	friend class TWeakPtr<T, ThreadSafe>;
	TSharedPtr(T* ptr, RefCounterType* counter): m_Ptr(ptr), m_Counter(counter){}
	void DecreaseRef() {
		if(!m_Counter->DecreaseSharedRef()) {
			delete m_Ptr;
			m_Ptr = nullptr;
		}
	}

	T* m_Ptr{ nullptr };
	RefCounterType* m_Counter{nullptr};
};

template<class T, bool ThreadSafe>
class TWeakPtr {
	using RefCounterType = TRawRefCounter<ThreadSafe>;
public:
	~TWeakPtr() {
		m_Counter->DecreaseWeakRef();
	}
	TWeakPtr() = default;
	TWeakPtr(const TSharedPtr<T, ThreadSafe>& rhs): m_Ptr(rhs.m_Ptr), m_Counter(rhs.m_Counter) {
		m_Counter->IncreaseWeakRef();
	}
	TWeakPtr(const TWeakPtr& rhs): m_Ptr(rhs.m_Ptr), m_Counter(rhs.m_Counter) {
		m_Counter->IncreaseWeakRef();
	}
	TWeakPtr(const TWeakPtr&&)noexcept = delete;
	TWeakPtr& operator=(const TWeakPtr& rhs) {
		if(this!=&rhs) {
			m_Counter->DecreaseWeakRef();
			rhs.m_Counter->IncreaseWeakRef();
			m_Counter = rhs.m_Counter;
		}
		return *this;
	}
	TWeakPtr& operator=(TWeakPtr&& rhs)noexcept = delete;

	TSharedPtr<T, ThreadSafe> Lock() {
		return TSharedPtr<T, ThreadSafe>{ m_Ptr, m_Counter };
	}

	bool IsValid() {
		return m_Counter->SharedRefCount() > 0;
	}

	T* Get() {
		return IsValid() ? m_Ptr : nullptr;
	}

private:
	T* m_Ptr{ nullptr };
	RefCounterType* m_Counter{nullptr};
};
