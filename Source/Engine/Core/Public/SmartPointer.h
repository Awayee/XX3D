#pragma once
#include <memory>

template <class T, class Dx=std::default_delete<T>>
using TUniquePtr = std::unique_ptr<T, Dx>;

template <class T, class ...Args>
inline TUniquePtr<T> MakeUniquePtr(Args...) {
	return std::make_unique<T>(Args...);
}

template <class T>
using TSharedPtr = std::shared_ptr<T>;

template <class T, class ...Args>
inline TSharedPtr<T> MakeSharedPtr(Args...) {
	return std::make_shared<T>(Args...);
}

template <class T>
using TWeakPtr = std::weak_ptr<T>;
