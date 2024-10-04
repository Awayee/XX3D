#include "Core/Public/File.h"
#include "Core/Public/Log.h"
#include "Core/Public/Defines.h"

namespace File {


	enum EFileMode: int {
		FM_AT_END = std::ios::ate,
		FM_BEGIN = std::ios::beg,
		FM_APPEND = std::ios::app,
		FM_BINARY = std::ios::binary,
		FM_IN = std::ios::in,
		FM_OUT = std::ios::out,
	};

	ReadFile::ReadFile(const char* file, bool isBinary): ReadFile(file, isBinary? FM_BINARY : 0) {}

	ReadFile::~ReadFile() {
		if(m_File.is_open()) {
			m_File.close();
		}
	}
	
	ReadFile::ReadFile(const char* file, int mode) {
		m_File.open(file, mode | FM_IN);
	}

	bool ReadFile::IsOpen() const {
		return m_File.is_open();
	}

	void ReadFile::Read(void* data, uint32 byteSize) {
		if(m_File.is_open()) {
			m_File.read((char*)data, byteSize);
		}
	}

	bool ReadFile::GetLine(XString& lineContent) {
		return !!std::getline(m_File, lineContent);
	}

	ReadFileWithSize::ReadFileWithSize(const char* file, bool isBinary): ReadFile(file, isBinary? (FM_AT_END | FM_BINARY) : FM_AT_END) {
		m_Size = (uint32)m_File.tellg();
		m_File.seekg(0, FM_BEGIN);
	}

	WriteFile::WriteFile(const char* file, bool isBinary): WriteFile(file, isBinary ? FM_BINARY : 0) {}

	WriteFile::~WriteFile() {
		if(m_File.is_open()) {
			m_File.close();
		}
	}

	WriteFile::WriteFile(const char* file, int fileMode) {
		m_File.open(file, fileMode | FM_OUT);
	}

	bool WriteFile::IsOpen() const {
		return m_File.is_open();
	}

	void WriteFile::Write(const void* data, uint32 byteSize) {
		if(m_File.is_open()) {
			m_File.write((const char*)data, byteSize);
		}
	}

	void ForeachPath(const char* folder, FForEachPathFunc&& func, bool recursively) {
		if(recursively) {
			DirRecursiveIterator iter{ folder };
			for (const DirEntry& path : iter) {
				func(path);
			}
		}
		else {
			DirIterator iter{ folder };
			for (const DirEntry& path : iter) {
				func(path);
			}			
		}
	}

	void ForeachPath(const DirEntry& path, FForEachPathFunc&& func, bool recursively) {
		if (recursively) {
			DirRecursiveIterator iter{ path };
			for (const DirEntry& p : iter) {
				func(p);
			}
		}
		else {
			DirIterator iter{ path };
			for (const DirEntry& p : iter) {
				func(p);
			}
		}
	}

}
