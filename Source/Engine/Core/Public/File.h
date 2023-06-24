#pragma once
#include <fstream>
#include <filesystem>
#include <functional>
#include "TVector.h"
#include "String.h"

namespace File {
	typedef std::ofstream Write;
	typedef std::ifstream Read;
	typedef std::fstream  FStream;
	typedef std::filesystem::path FPath;
	typedef std::filesystem::directory_entry FPathEntry;
	typedef std::filesystem::directory_iterator FPathIterator;
	typedef std::filesystem::recursive_directory_iterator FPathRecursiveIterator;
	typedef const char* PathStr;

	void LoadFileCode(const char* file, TVector<char>& code);

	inline FPath RelativePath(const FPath& path, const FPath& base) {
		return std::filesystem::relative(path, base);
	}

	inline bool IsFolder(const FPath& path) {
		return std::filesystem::is_directory(path);
	}

	inline bool Exist(const FPath& path) {
		return std::filesystem::exists(path);
	}

	typedef std::function<void(const FPathEntry&)> FForEachPathFunc;
	void ForeachPath(const char* folder, FForEachPathFunc&& func, bool recursively = false);
	void ForeachPath(const FPathEntry& path, FForEachPathFunc&& func, bool recursively = false);
}

