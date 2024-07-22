#pragma once
#include "Concurrency.h"
#include "TUniquePtr.h"
#include "Defines.h"

// a singleton template, is not thread safe.
template<typename T> class TSingleton {
public:
	template <typename...Args>
	static void Initialize(Args...args) {
		Release();
		s_Instance = new T(args...);
	}
	static void Release() {
		delete s_Instance;
	}
	static T* Instance() {
		return s_Instance;
	}
protected:
	static T* s_Instance;
	TSingleton() = default;
	virtual ~TSingleton() = default;
	NON_COPYABLE(TSingleton);
	NON_MOVEABLE(TSingleton);
};

template<typename T> T* TSingleton<T>::s_Instance;

