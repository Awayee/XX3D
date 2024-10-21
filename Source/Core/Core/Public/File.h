#pragma once
#include <fstream>
#include <filesystem>
#include <functional>
#include "TArray.h"
#include "String.h"

namespace File {
	typedef std::filesystem::path FPath;
	typedef std::filesystem::directory_entry DirEntry;
	typedef std::filesystem::directory_iterator DirIterator;
	typedef std::filesystem::recursive_directory_iterator DirRecursiveIterator;
	typedef const char* PathStr;

	class ReadFile {
	public:
		NON_COPYABLE(ReadFile);
		NON_MOVEABLE(ReadFile);
		ReadFile(const char* file, bool isBinary);
		ReadFile(const XString& file, bool isBinary): ReadFile(file.c_str(), isBinary){}
		~ReadFile();
		bool IsOpen() const;
		void Read(void* data, uint32 byteSize);
		bool GetLine(XString& lineContent);
	protected:
		ReadFile(const char* file, int mode);
		std::ifstream m_File;
	};

	class ReadFileWithSize: public ReadFile {
	public:
		ReadFileWithSize(const char* file, bool isBinary);
		ReadFileWithSize(const XString& file, bool isBinary) : ReadFileWithSize(file.c_str(), isBinary) {}
		uint32 ByteSize() const { return m_Size; }
	private:
		uint32 m_Size;
	};

	class WriteFile {
	public:
		NON_COPYABLE(WriteFile);
		NON_MOVEABLE(WriteFile);
		WriteFile(const char* file, bool isBinary);
		WriteFile(const XString& file, bool isBinary): WriteFile(file.c_str(), isBinary){}
		~WriteFile();
		bool IsOpen() const;
		void Write(const void* data, uint32 byteSize);
	protected:
		WriteFile(const char* file, int mode);
		std::ofstream m_File;
	};

	inline FPath RelativePath(const FPath& path, const FPath& base) {
		if(path == base) {
			return {};
		}
		return std::filesystem::relative(path, base);
	}

	// whether "path" is sub path of "base"
	inline bool IsSubPathOf(const FPath& path, const FPath& base) {
		const FPath relativePath = RelativePath(path, base);
		return !relativePath.empty() && relativePath.wstring().find(L"..") != XString::npos;
	};

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

	inline bool Rename(const FPath& path, const FPath& newPath) {
		try {
			std::filesystem::rename(path, newPath);
		}
		catch(const std::filesystem::filesystem_error&) {
			return false;
		}
		return true;
	}

	typedef std::function<void(const DirEntry&)> FForEachPathFunc;
	void ForeachPath(const char* folder, FForEachPathFunc&& func, bool recursively = false);
	void ForeachPath(const DirEntry& path, FForEachPathFunc&& func, bool recursively = false);

	inline FPath GetExecutablePath() {
		return std::filesystem::current_path();
	}
}

