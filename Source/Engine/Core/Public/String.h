#pragma once
#include <string>

typedef std::string String;

bool StartsWith(const char* str, const char* sign);

bool EndsWith(const char* str, const char* sign);

template <typename ...T>
String StringFormat(const char* str, T ...args) {
	char strBuf[128];
	sprintf(strBuf, str, args...);
	return String(strBuf);
}

template<typename T>
inline String ToString(T val) {
	return std::to_string(val);
}