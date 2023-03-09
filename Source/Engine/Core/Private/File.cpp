#include "Core/Public/File.h"
#include "Core/Public/macro.h"
#include "Core/Public/TypeDefine.h"

#define PARSE_ASSETS_FILE(f)\
	char __s[128]; strcpy(__s, PROJECT_ASSETS); strcat(__s, f); f=__s

namespace File {

	void LoadFileCode(const char* file, TVector<char>& code) {
		std::ifstream f(file, std::ios::ate | std::ios::binary);
		ASSERT(f.is_open(), "file load failed!");

		uint32 fileSize = (uint32)f.tellg();
		code.resize(fileSize);
		f.seekg(0);
		f.read(code.data(), fileSize);
		f.close();
	}

	// lod .ini file

	bool LoadIniFile(const char* file, TUnorderedMap<String, String>& configMap) {
		std::ifstream configFile(file);
		if (!configFile.is_open()) {
			return false;
		}
		String fileLine;
		configMap.clear();
		while (std::getline(configFile, fileLine)) {
			if (fileLine.empty() || fileLine[0] == '#') {
				continue;
			}
			uint32 separate = fileLine.find_first_of('=');
			if (separate > 0 && separate < fileLine.length() - 1) {
				String name = fileLine.substr(0, separate);
				String value = fileLine.substr(separate + 1, fileLine.length() - separate - 1);
				configMap.insert({ std::move(name), std::move(value) });
			}
		}
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
