#pragma once
#include <combaseapi.h>
#include <dxcapi.h>
#include <vector>

class SPVCompiler {
private:
	bool m_Initialized{ false };
	IDxcLibrary* m_Library{nullptr};
	IDxcCompiler3* m_Compiler{nullptr};
	IDxcUtils* m_Utils{nullptr};
public:
	SPVCompiler();
	~SPVCompiler();
	bool CompileSingleEntry(const wchar_t* hlslFile, const wchar_t* entryPoint, const wchar_t* shaderModel, const std::vector<DxcDefine>& defines, const char* outFile);
	bool CompileHLSL(const char* hlslFile);
};
