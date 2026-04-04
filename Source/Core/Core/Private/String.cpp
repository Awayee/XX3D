#include "Core/Public/String.h"
#include "Windows.h"

XWString String2WString(const XString& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size_needed);
    return wstr;
}

XString WString2String(const XWString& wStr) {
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wStr.c_str(), (int)wStr.size(), NULL, 0, NULL, NULL);
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wStr.c_str(), (int)wStr.size(), &str[0], size_needed, NULL, NULL);
    return str;
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
