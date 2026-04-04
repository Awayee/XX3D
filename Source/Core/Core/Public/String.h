#pragma once
#include <string>
#include <string_view>
#include <charconv>

typedef std::string XString;
typedef std::wstring XWString;
typedef std::string_view XStringView;
typedef std::wstring_view XWStringView;

XWString String2WString(const XString& str);

XString WString2String(const XWString& wStr);

bool StrStartsWith(XStringView Str, XStringView Sign);

bool StrStartsWith(XStringView Str, char Ch);

bool StrEndsWith(XStringView Str, XStringView Sign);

bool StrEndsWith(XStringView Str, char Ch);

inline bool StrStartsWith(const char* Str, const char* Sign) {
	return StrStartsWith(XStringView{ Str }, XStringView{ Sign });
}

inline bool StrEndsWith(const char* Str, const char* Sign) {
	return StrEndsWith(XStringView{ Str }, XStringView{ Sign });
}

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
inline T StringToNum(XStringView str) {
	T Value;
	std::from_chars(str.data(), str.data() + str.size(), Value);
	return Value;
}

template<> inline float StringToNum(XStringView str) {
	float Value;
	std::from_chars(str.data(), str.data() + str.size(), Value);
	return Value;
}

template<> inline double StringToNum(XStringView str) {
	double Value;
	std::from_chars(str.data(), str.data() + str.size(), Value);
	return Value;
}

template<> inline bool StringToNum(XStringView str) {
	int Value;
	std::from_chars(str.data(), str.data() + str.size(), Value);
	return (bool)Value;
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