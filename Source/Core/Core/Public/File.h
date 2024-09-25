#pragma once
#include <fstream>
#include <filesystem>
#include <functional>
#include "TArray.h"

namespace File {
	typedef std::ofstream WFile;
	typedef std::ifstream RFile;
	typedef std::fstream  RWFile;
	typedef std::filesystem::path FPath;
	typedef std::filesystem::directory_entry FPathEntry;
	typedef std::filesystem::directory_iterator FPathIterator;
	typedef std::filesystem::recursive_directory_iterator FPathRecursiveIterator;
	typedef const char* PathStr;

	enum EFileMode {
		AtEnd = std::ios::ate,
		Append = std::ios::app,
		Binary = std::ios::binary,
		Read = std::ios::in,
		Write = std::ios::out,
	};

	bool LoadFileCode(const char* file, TArray<char>& code);

	inline FPath RelativePath(const FPath& path, const FPath& base) {
		if(path == base) {
			return {};
		}
		return std::filesystem::relative(path, base);
	}

	inline bool IsFolder(const FPath& path) {
		return std::filesystem::is_directory(path);
	}

	inline bool Exist(const FPath& path) {
		return std::filesystem::exists(path);
	}

	inline void RemoveFile(const FPath& path) {
		std::filesystem::remove(path);
	}

	inline void RemoveDir(const FPath& path) {
		std::filesystem::remove_all(path);
	}

	inline void MakeDir(const FPath& path) {
		std::filesystem::create_directory(path);
	}

	typedef std::function<void(const FPathEntry&)> FForEachPathFunc;
	void ForeachPath(const char* folder, FForEachPathFunc&& func, bool recursively = false);
	void ForeachPath(const FPathEntry& path, FForEachPathFunc&& func, bool recursively = false);
}

