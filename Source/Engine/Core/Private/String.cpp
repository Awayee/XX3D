#include "Core/Public/String.h"

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
}

inline bool StrEndsWith(const char* str, const char* sign) {
	if(!str || ! sign) {
		return false;
	}

	auto strLen = strlen(str);
	auto signLen = strlen(sign);
	if(strLen < signLen) {
		return false;
	}
	return strncmp(str + (strLen - signLen), sign, signLen) == 0;
}
