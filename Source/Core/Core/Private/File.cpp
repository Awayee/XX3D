#include "Core/Public/File.h"
#include "Core/Public/Log.h"
#include "Core/Public/Defines.h"

namespace File {

	bool LoadFileCode(const char* file, TArray<char>& code) {
		RFile f(file, EFileMode::AtEnd | EFileMode::Binary);
		if(!f.is_open()) {
			return false;
		}

		uint32 fileSize = (uint32)f.tellg();
		code.Resize(fileSize);
		f.seekg(0);
		f.read(code.Data(), fileSize);
		f.close();
		return true;
	}

	void ForeachPath(const char* folder, FForEachPathFunc&& func, bool recursively) {
		if(recursively) {
			FPathRecursiveIterator iter{ folder };
			for (const FPathEntry& path : iter) {
				func(path);
			}
		}
		else {
			FPathIterator iter{ folder };
			for (const FPathEntry& path : iter) {
				func(path);
			}			
		}
	}

	void ForeachPath(const FPathEntry& path, FForEachPathFunc&& func, bool recursively) {
		if (recursively) {
			FPathRecursiveIterator iter{ path };
			for (const FPathEntry& p : iter) {
				func(p);
			}
		}
		else {
			FPathIterator iter{ path };
			for (const FPathEntry& p : iter) {
				func(p);
			}
		}
	}

}
