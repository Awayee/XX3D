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
