#pragma once
#include "Concurrency.h"
template<typename T>
class TSingleton {
public:
	template <typename...Args>
	static void Initialize(Args...args) {
		if(s_Instance) {
			MutexLock lock(s_Mutex);
			if (s_Instance) {
				delete s_Instance;
			}
		}
		if(!s_Instance) {
			MutexLock lock(s_Mutex);
			if(!s_Instance) {
				s_Instance = new T(args...);
			}
		}
	}
	static void Release() {
		if (nullptr != s_Instance) {
			delete s_Instance;
			s_Instance = nullptr;
		}
	}
	static T* Instance() {
		return s_Instance;
	}
	static T* Get() {
		return s_Instance;
	}
protected:
	static T* s_Instance;
	static Mutex s_Mutex;
	TSingleton(const TSingleton<T>&) = delete;
	TSingleton(TSingleton<T>&&) = delete;
	TSingleton() = default;
	virtual ~TSingleton() = default;
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