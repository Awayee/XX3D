#pragma once
#include <atomic>
#include "Core/Public/TypeDefine.h"

template <bool ThreadSafe>
struct TRefCounterTypeTemplate {
	using Type = uint32;
};

template<>
struct TRefCounterTypeTemplate<true> {
	using Type = std::atomic_uint32_t;
};

// A wrapped reference counter with shared ref count and weak ref count.
// The controlled object could be destroyed when SharedRefCount is zero; this object would be destroyed when WeakRefCount is zero.
template<bool ThreadSafe>
class TRawRefCounter {
	using RefCounterType = typename TRefCounterTypeTemplate<ThreadSafe>::Type;
public:
	~TRawRefCounter() = default;
	//The last weak ptr controls the life span of object, destroyed when m_WeakRefCount==0
	TRawRefCounter():m_SharedRefCount(1),m_WeakRefCount(1){}
	TRawRefCounter(const TRawRefCounter&) = delete;
	TRawRefCounter(TRawRefCounter&&)noexcept = delete;
	TRawRefCounter& operator=(const TRawRefCounter&) = delete;
	TRawRefCounter& operator=(TRawRefCounter&&)noexcept = delete;

	uint32 SharedRefCount() {
		if(ThreadSafe) {
			return m_SharedRefCount.load(std::memory_order_relaxed);
		}
		return m_SharedRefCount;
	}
	uint32 WeakRefCount() {
		if constexpr (ThreadSafe) {
			return m_WeakRefCount.load(std::memory_order_relaxed);
		}
		return m_WeakRefCount;
	}
	void IncreaseSharedRef() {
		if constexpr (ThreadSafe) {
			m_SharedRefCount.fetch_add(1, std::memory_order_release);
		}
		else {
			++m_SharedRefCount;
		}
	}
	void IncreaseWeakRef() {
		if constexpr (ThreadSafe) {
			m_WeakRefCount.fetch_add(1, std::memory_order_release);
		}
		else {
			++m_WeakRefCount;
		}
	}

	uint32 DecreaseSharedRef() {
		uint32 restCount;
		if constexpr (ThreadSafe) {
			restCount = m_SharedRefCount.fetch_sub(1, std::memory_order_release) - 1;
		}
		else {
			restCount = --m_SharedRefCount;
		}
		if(0 == restCount) {
			DecreaseWeakRef();
		}
		return restCount;
	}

	uint32 DecreaseWeakRef() {
		uint32 restCount;
		if constexpr (ThreadSafe) {
			restCount = m_WeakRefCount.fetch_sub(1, std::memory_order_release) - 1;
		}
		else {
			restCount = --m_WeakRefCount;
		}
		if (0 == restCount) {
			delete this;
		}
		return restCount;
	}
private:
	RefCounterType m_SharedRefCount{1};
	RefCounterType m_WeakRefCount{1};
};

template<bool ThreadSafe>
class TWeakRefCounter;

// shared reference counter, operate shared ref count by passing value
template<bool ThreadSafe>
class TSharedRefCounter {
	using RawRefCounterType = TRawRefCounter<ThreadSafe>;
public:
	~TSharedRefCounter() {
		m_Counter->DecreaseSharedRef();
	}
	TSharedRefCounter() {
		m_Counter = new RawRefCounterType();
	}

	TSharedRefCounter(const TSharedRefCounter& rhs) : m_Counter(rhs.m_Counter) {
		m_Counter->IncreaseSharedRef();
	}

	TSharedRefCounter(TSharedRefCounter&& rhs) noexcept: m_Counter(rhs.m_Counter) {
		rhs.m_Counter = nullptr;
	}

	TSharedRefCounter(const TWeakRefCounter<ThreadSafe>& rhs): m_Counter(rhs.m_Counter) {
		m_Counter->IncreaseSharedRef();
	}

	TSharedRefCounter& operator=(const TSharedRefCounter& rhs) {
		if(this != &rhs) {
			m_Counter->DecreaseSharedRef();
			rhs.m_Counter->IncreaseSharedRef();;
			m_Counter = rhs.m_Counter;
		}
		return *this;
	}

	TSharedRefCounter& operator=(TSharedRefCounter&& rhs) noexcept {
		m_Counter = rhs.m_Counter;
		rhs.m_Counter = nullptr;
		return *this;
	}
private:
	RawRefCounterType* m_Counter;
};

// weak reference counter, operate weak ref count by passing value
template<bool ThreadSafe>
class TWeakRefCounter {
	using RawRefCounterType = TRawRefCounter<ThreadSafe>;
public:
	TWeakRefCounter(): m_Counter(nullptr) { }

	TWeakRefCounter(const TWeakRefCounter& rhs): m_Counter(rhs.m_Counter) {
		m_Counter->IncreaseWeakRef();
	}

	TWeakRefCounter(TWeakRefCounter&& rhs) noexcept : m_Counter(rhs.m_Counter) {
		rhs.m_Counter = nullptr;
	}

	TWeakRefCounter(const TSharedRefCounter<ThreadSafe>& rhs) : m_Counter(rhs.m_Counter) {
		m_Counter->IncreaseWeakRef();
	}

	TWeakRefCounter(TSharedRefCounter<ThreadSafe>&& rhs) noexcept : m_Counter(rhs.m_Counter) {
		rhs.m_Counter = nullptr;
	}

	TWeakRefCounter& operator=(const TWeakRefCounter& rhs) {
		if(m_Counter != rhs.m_Counter) {
			m_Counter->DecreaseWeakRef();
			rhs.m_Counter->IncreaseWeakRef();
			m_Counter = rhs.m_Counter;
		}
		return *this;
	}

	TWeakRefCounter& operator=(TWeakRefCounter&& rhs) noexcept{
		m_Counter = rhs.m_Counter;
		rhs.m_Counter = nullptr;
		return *this;
	}

	TWeakRefCounter& operator=(const TSharedRefCounter<ThreadSafe>& rhs) {
		if(m_Counter != rhs.m_Counter) {
			m_Counter->DecreaseWeakRef();
			rhs.m_Counter->IncreaseWeakRef();
			m_Counter = rhs.m_Counter;
		}
		return *this;
	}

	TWeakRefCounter& operator=(TSharedRefCounter<ThreadSafe>&& rhs) noexcept {
		m_Counter = rhs.m_Counter;
		rhs.m_Counter = nullptr;
		return *this;
	}
private:
	friend TSharedRefCounter<ThreadSafe>;
	RawRefCounterType* m_Counter;
};
