#pragma once
#include <fstream>
#include <filesystem>
#include <functional>
#include "Container.h"
#include "String.h"

namespace File {
	typedef std::ofstream Write;
	typedef std::ifstream Read;
	typedef std::fstream  FStream;
	typedef std::filesystem::path FPath;
	typedef std::filesystem::directory_entry FPathEntry;
	typedef std::filesystem::directory_iterator FPathIterator;
	typedef std::filesystem::recursive_directory_iterator FPathRecursiveIterator;

	void LoadFileCode(const char* file, TVector<char>& code);

	typedef std::function<void(const FPathEntry&)> FForEachPathFunc;
	void ForeachPath(const char* folder, FForEachPathFunc&& func, bool recursively = false);
	void ForeachPath(const FPathEntry& path, FForEachPathFunc&& func, bool recursively = false);
}

