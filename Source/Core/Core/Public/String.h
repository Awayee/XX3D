#pragma once
#include <string>

typedef std::string XXString;

inline bool StrStartsWith(const char* str, const char* sign) {
	if (!str || !sign) {
		return false;
	}
	auto strLen = strlen(str);
	auto signLen = strlen(sign);
	if (strLen < signLen) {
		return false;
	}
	return strncmp(str, sign, signLen) == 0;
};

inline bool StrEndsWith(const char* str, const char* sign) {
	if (!str || !sign) {
		return false;
	}

	auto strLen = strlen(str);
	auto signLen = strlen(sign);
	if (strLen < signLen) {
		return false;
	}
	return strncmp(str + (strLen - signLen), sign, signLen) == 0;
};

inline bool StrEqual(const char* s0, const char* s1) {
	return strcmp(s0, s1) == 0;
}

template <unsigned int Num=128, typename ...T>
XXString StringFormat(const char* str, T ...args) {
	char strBuf[Num];
	sprintf(strBuf, str, args...);
	return XXString(strBuf);
}

template<typename T>
inline XXString ToString(T val) {
	return std::to_string(val);
}

inline void StrCopy(char* dst, const char* src) {
	std::strcpy(dst, src);
}

inline void StrAppend(char* str0, const char* str1) {
	std::strcat(str0, str1);
}

inline bool StrEmpty(const char* str) {
	return nullptr == str || '\0' == *str;
}