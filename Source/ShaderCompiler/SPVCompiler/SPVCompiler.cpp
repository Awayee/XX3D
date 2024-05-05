#include "SPVCompiler.h"
#include <iostream>
#include <vector>
#include <Windows.h>
#include <memory>
#include <atlbase.h>
#include <fstream>
#include <filesystem>

#define SHADER_MODEL_VS L"vs_6_1"
#define SHADER_MODEL_PS L"ps_6_1"
#define ENTRY_POINT_VS L"MainVS"
#define ENTRY_POINT_PS L"MainPS"
#define SPV_EXT L"spv"

#define DX_RELEASE(x) if(x){ (x)->Release(); (x)=nullptr;}

#define STR_LEN_MAX 128

//#define PARSE_SHADER_PATH(x)\
//char x##__s[STR_LEN_MAX];{\
//	int pathSize = strlen(SHADER_PATH);\
//	if (pathSize > STR_LEN_MAX - 1) pathSize = STR_LEN_MAX - 1;\
//	strcpy_s(x##__s, pathSize, SHADER_PATH);\
//	x##__s[pathSize] = '\0';\
//	int fSize = strlen(x);\
//	if (pathSize + fSize > STR_LEN_MAX - 1) fSize = STR_LEN_MAX - 1 - pathSize;\
//	if (fSize > 0) {\
//		strcat_s(x##__s, fSize, x);\
//		x##__s[fSize + pathSize] = 0;\
//	}}x = x##__s

#define PARSE_SHADER_PATH(x)\
	char x##__s[STR_LEN_MAX];{\
		int l = strlen(SHADER_PATH) + 1;\
		if (l > STR_LEN_MAX) l = STR_LEN_MAX;\
		strcpy_s(x##__s, l, SHADER_PATH);\
		x##__s[l - 1] = '\0';\
		if (l + strlen(x) + 1 <= STR_LEN_MAX)strcat_s(x##__s, sizeof(x##__s), x);\
	}x=x##__s

#define PARSE_SHADER_PATH_W(x)\
	wchar_t x##W[STR_LEN_MAX];{\
		int pathSize = MultiByteToWideChar(CP_OEMCP, 0, SHADER_PATH, strlen(SHADER_PATH), NULL, 0);\
		if (pathSize > STR_LEN_MAX-1) pathSize = STR_LEN_MAX-1;\
		MultiByteToWideChar(CP_OEMCP, 0, SHADER_PATH, strlen(SHADER_PATH), x##W, pathSize);\
		int fSize = MultiByteToWideChar(CP_OEMCP, 0, x, strlen(x), NULL, 0);\
		if (pathSize + fSize > STR_LEN_MAX-1)fSize = STR_LEN_MAX-1 - pathSize;\
		if (fSize > 0)MultiByteToWideChar(CP_OEMCP, 0, x, strlen(x), x##W + pathSize, fSize);\
		x##W[pathSize + fSize] = '\0';\
	}



SPVCompiler::SPVCompiler() {
	HRESULT r;
	// Initialize DXC library
	r = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&m_Library));
	if (FAILED(r)) {
		printf("[HLSL2Spv] Could not init DXC Library\n");
		return;
	}
	// Initialize DXC compiler
	r = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_Compiler));
	if (FAILED(r)) {
		printf("[HLSL2Spv] Could not init DXC Compiler\n");
		return;
	}

	// Initialize DXC utility
	r = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_Utils));
	if (FAILED(r)) {
		printf("[HLSL2Spv] Could not init DXC Utiliy\n");
		return;
	}
	m_Initialized = true;
}

SPVCompiler::~SPVCompiler() {
	DX_RELEASE(m_Compiler);
	DX_RELEASE(m_Utils);
	DX_RELEASE(m_Library);
}

bool SPVCompiler::CompileSingleEntry(const wchar_t* hlslFile, const wchar_t* entryPoint, const wchar_t* shaderModel, const std::vector<DxcDefine>& defines, const char* outFile) {

	HRESULT r;
	// Load the HLSL text shader from disk
	uint32_t codePage = DXC_CP_ACP;
	CComPtr<IDxcBlobEncoding> sourceBlob;
	r = m_Utils->LoadFile(hlslFile, &codePage, &sourceBlob);
	if (FAILED(r)) {
		wprintf(L"[HLSL2Spv] Could not load shader file: %s\n", hlslFile);
		return false;
	}

	// compile vs

	CComPtr<IDxcCompilerArgs> args;
	m_Utils->BuildArguments(hlslFile, entryPoint, shaderModel, nullptr, 0, defines.data(), defines.size(), &args);
	LPCWSTR spirv = L"-spirv";
	args->AddArguments(&spirv, 1);

	std::vector<LPCWSTR> arguments = {
		// (Optional) name of the shader file to be displayed e.g. in an error message
		hlslFile,
		// Shader main entry point
		L"-E", entryPoint,
		// Shader target profile
		L"-T", shaderModel,
		// Compile to SPIRV
		L"-spirv"
	};

	DxcBuffer buffer;
	buffer.Encoding = DXC_CP_ACP;
	buffer.Ptr = sourceBlob->GetBufferPointer();
	buffer.Size = sourceBlob->GetBufferSize();

	CComPtr<IDxcResult> result{ nullptr };
	r = m_Compiler->Compile(
		&buffer,
		args->GetArguments(),
		args->GetCount(),
		nullptr,
		IID_PPV_ARGS(&result)
	);

	if (SUCCEEDED(r)) {
		result->GetStatus(&r);
	}

	// Output error if compilation failed
	if (FAILED(r) && (result)) {
		CComPtr<IDxcBlobEncoding> errorBlob;
		r = result->GetErrorBuffer(&errorBlob);
		if (SUCCEEDED(r) && errorBlob) {
			std::cerr << "Shader compilation failed :\n\n" << (const char*)errorBlob->GetBufferPointer();
			return false;
		}
	}

	// Get compilation result
	CComPtr<IDxcBlob> code;
	result->GetResult(&code);

	std::ofstream fout(outFile, std::ios::binary | std::ios::out);
	if (!fout.is_open()) {
		printf("[HLSL2Spv] write file failed: %s\n", outFile);
		return false;
	}
	fout.write((char*)code->GetBufferPointer(), code->GetBufferSize());
	fout.close();
	return true;
}

bool SPVCompiler::CompileHLSL(const char* hlslFile) {
	PARSE_SHADER_PATH(hlslFile);
	printf("[ShaderCompile] Compiling Shader: %s\n", hlslFile);
	std::filesystem::path hlslPath(hlslFile);

	// out vs
	std::filesystem::path vsPath = hlslPath;
	vsPath.replace_extension("");
	vsPath += ENTRY_POINT_VS;
	vsPath.replace_extension(L"spv");
	bool vs = CompileSingleEntry(hlslPath.c_str(), ENTRY_POINT_VS, SHADER_MODEL_VS, {{L"_VS"}}, vsPath.string().c_str());
	// out ps
	std::filesystem::path psPath = hlslPath;
	psPath.replace_extension("");
	psPath += ENTRY_POINT_PS;
	psPath.replace_extension(L"spv");
	bool ps = CompileSingleEntry(hlslPath.c_str(), ENTRY_POINT_PS, SHADER_MODEL_PS, { {L"_PS"}}, psPath.string().c_str());

	return vs && ps;
}
