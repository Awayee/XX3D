#pragma once
#include <memory>

template <class T>
using TSharedPtr = std::shared_ptr<T>;

template <class T, class ...Args>
inline TSharedPtr<T> MakeSharedPtr(Args...args) {
	return std::make_shared<T>(args...);
}

template <class T>
using TWeakPtr = std::weak_ptr<T>;
