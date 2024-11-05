#pragma once
#include <string>
#include <cwchar>

typedef std::string XString;
typedef std::wstring XWString;

XWString String2WString(const XString& str);

XString WString2String(const XWString& wStr);

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
XString StringFormat(const char* str, T ...args) {
	XString strBuf;
	strBuf.resize(Num);
	const int strLen = sprintf_s(strBuf.data(), Num, str, args...);
	strBuf.resize(strLen);
	return strBuf;
}

template<unsigned int Num=128, typename ...T>
XWString WStringFormat(const wchar_t* str, T...args) {
	XWString strBuf;
	strBuf.resize(Num);
	const int strLen = swprintf_s(strBuf.data(), Num, str, args...);
	strBuf.resize(strLen);
	return strBuf;
}

template<typename T>
inline XString ToString(T val) {
	return std::to_string(val);
}

template<typename T>
inline T StringToNum(const char* str) {
	return (T)std::atoi(str);
}

template<> inline float StringToNum(const char* str) {
	return (float)std::atof(str);
}

template<> inline double StringToNum(const char* str) {
	return (double)std::atof(str);
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

inline bool StrContains(const char* str, const char* subStr) {
	return std::strstr(str, subStr) != nullptr;
}

inline size_t StringSize(const char* str) {
	return std::strlen(str);
}