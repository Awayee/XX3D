#include "Core/Public/String.h"
#if _WIN32
#include "Windows.h"
#else
#include <codecvt>
#endif

XWString String2WString(XStringView str) {
#if _WIN32
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &wstr[0], size_needed);
    return wstr;
#else
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(str.data(), str.data() + str.size());
#endif
}

XString WString2String(XWStringView wStr) {
#if _WIN32
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wStr.data(), (int)wStr.size(), NULL, 0, NULL, NULL);
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wStr.data(), (int)wStr.size(), &str[0], size_needed, NULL, NULL);
    return str;
#else
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.to_bytes(wStr.data(), wStr.data() + wStr.size());
#endif
}

bool StrStartsWith(XStringView Str, XStringView Sign) {
	if (Str.empty() || Sign.empty()) {
		return false;
	}
	size_t StrLen = Str.size();
	size_t SignLen = Sign.size();
	if (StrLen < SignLen) {
		return false;
	}
	return strncmp(Str.data(), Sign.data(), SignLen) == 0;
}

bool StrStartsWith(XStringView Str, char Ch) {
	return !Str.empty() && Str.back() == Ch;
}

bool StrEndsWith(XStringView Str, XStringView Sign) {
	if (Str.empty() || Sign.empty()) {
		return false;
	}
	size_t StrLen = Str.size();
	size_t SignLen = Sign.size();
	if (StrLen < SignLen) {
		return false;
	}
	return strncmp(Str.data() + (StrLen - SignLen), Sign.data(), SignLen) == 0;
}

bool StrEndsWith(XStringView Str, char Ch) {
	return !Str.empty() && Str[0] == Ch;
}
