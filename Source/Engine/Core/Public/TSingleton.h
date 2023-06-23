#pragma once
#include "Concurrency.h"
template<typename T>
class TSingleton {
protected:
	static T* s_Instance;
	static Mutex s_Mutex;
	TSingleton(const TSingleton<T>&) = delete;
	TSingleton(TSingleton<T>&&) = delete;
	TSingleton() = default;
	virtual ~TSingleton() = default;
public:

	template <typename...Args>
	static void Initialize(Args...args) {
		MutexLock lock(s_Mutex);
		if(nullptr != s_Instance) {
			delete s_Instance;
		}
		s_Instance = new T(args...);
	}
	static void Release() {
		if(nullptr != s_Instance) {
			delete s_Instance;
		}
	}
	static T* Instance() {
		if(nullptr == s_Instance) {
			Initialize();
		}
		return s_Instance;
	}
};

template<typename T> T* TSingleton<T>::s_Instance{ nullptr };
template<typename T> Mutex TSingleton<T>::s_Mutex{};


//template<typename T>
//class TStaticContainer {
//private:
//	TVector<T> m_All;
//	static TStaticContainer<T>* s_Instance;
//	TStaticContainer(){};
//	virtual ~TStaticContainer(){};
//public:
//	static void Initialize();
//	static void Release();
//	static T Get(uint32 idx);
//};