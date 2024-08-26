#include "SPVCompiler.h"
#include "Core/Public/File.h"
#include "Core/Public/Container.h"
#include <Windows.h>

#define SHADER_MODEL_VS L"vs_6_1"
#define SHADER_MODEL_PS L"ps_6_1"
#define SHADER_MODEL_CS L"cs_6_1"

#define DX_RELEASE(x) if(x){ (x)->Release(); (x)=nullptr;}
#define COMPILE_FILE_EXT ".spv"
#define STR_LEN_MAX 128

File::FPath s_ShaderRoot = SHADER_PATH;

inline const wchar_t* GetShaderModule(ESPVShaderStage stage) {
	switch(stage) {
	case ESPVShaderStage::Vertex:
		return SHADER_MODEL_VS;
	case ESPVShaderStage::Pixel:
		return SHADER_MODEL_PS;
	case ESPVShaderStage::Compute:
		return SHADER_MODEL_CS;
	default:
		return L"";
	}
}

class CustomIncludeHandler : public IDxcIncludeHandler{
public:
	HRESULT STDMETHODCALLTYPE LoadSource(_In_ LPCWSTR pFilename, _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource) override
	{
		IDxcBlobEncoding* pEncoding;
		std::string path = WString2String(pFilename);
		if (IncludedFiles.find(path) != IncludedFiles.end())
		{
			// Return empty string blob if this file has been included before
			static const char nullStr[] = " ";
			pUtils->CreateBlobFromPinned(nullStr, ARRAYSIZE(nullStr), DXC_CP_ACP, &pEncoding);
			*ppIncludeSource = pEncoding;
			return S_OK;
		}

		HRESULT hr = pUtils->LoadFile(pFilename, nullptr, &pEncoding);
		if (SUCCEEDED(hr))
		{
			IncludedFiles.insert(path);
			*ppIncludeSource = pEncoding;
		}
		return hr;
	}
	ULONG STDMETHODCALLTYPE AddRef() override {
		return InterlockedIncrement(&m_refCount);
	}

	ULONG STDMETHODCALLTYPE Release() override {
		ULONG refCount = InterlockedDecrement(&m_refCount);
		if (refCount == 0) {
			delete this;
		}
		return refCount;
	}
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override {
		if (riid == __uuidof(IUnknown) || riid == __uuidof(IDxcIncludeHandler)) {
			*ppvObject = static_cast<IDxcIncludeHandler*>(this);
			AddRef();
			return S_OK;
		}
		*ppvObject = nullptr;
		return E_NOINTERFACE;
	}
private:
	ULONG m_refCount = 1;
	IDxcUtils* pUtils;
	std::unordered_set<std::string> IncludedFiles;
};

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
}

SPVCompiler::~SPVCompiler() {
	DX_RELEASE(m_Compiler);
	DX_RELEASE(m_Utils);
	DX_RELEASE(m_Library);
}

XString SPVCompiler::GetShaderOutputFile(const XString& hlslFile, const XString& entryPoint) {
	XString resultFile;
	if (auto extIndex = hlslFile.rfind(".hlsl"); extIndex != hlslFile.npos) {
		resultFile = hlslFile.substr(0, extIndex);
	}
	else {
		resultFile = hlslFile;
	}
	resultFile.append(entryPoint);
	resultFile.append(COMPILE_FILE_EXT);
	return resultFile;
}

void SPVCompiler::ClearCompiledCache() {
	File::ForeachPath(SHADER_PATH, [](const File::FPathEntry& p) {
		if(p.path().extension()== COMPILE_FILE_EXT){
			File::RemoveFile(p.path());
		}
	}, true);
}

bool SPVCompiler::CompileHLSL(const XString& hlslFile, const XString& entryPoint, ESPVShaderStage stage, TConstArrayView<SPVDefine> defines) {
	File::FPath hlslFilePath = s_ShaderRoot;
	hlslFilePath.append(hlslFile);
	const wchar_t* hlslFileW = hlslFilePath.c_str();
	
	File::FPath outFilePath{ SHADER_PATH };
	outFilePath.append(GetShaderOutputFile(hlslFile, entryPoint));

	HRESULT r;
	// Load the HLSL text shader from disk
	uint32_t codePage = DXC_CP_ACP;
	IDxcBlobEncoding* sourceBlob;
	r = m_Utils->LoadFile(hlslFileW, &codePage, &sourceBlob);
	if (FAILED(r)) {
		LOG_ERROR("[HLSL2Spv] Could not load shader file: %s", hlslFile.c_str());
		return false;
	}

	const wchar_t* shaderModel = GetShaderModule(stage);
	IDxcCompilerArgs* args;
	// parse defines
	TArray<TPair<XWString, XWString>> definesW; definesW.Reserve(defines.Size());
	TArray<DxcDefine> dxcDefines; dxcDefines.Reserve(defines.Size());
	for(const auto& define: defines) {
		auto& defineW = definesW.EmplaceBack();
		defineW.first = String2WString(define.Name);
		defineW.second = String2WString(define.Value);
		dxcDefines.PushBack({ defineW.first.c_str(), defineW.second.c_str() });
	}
	//CustomIncludeHandler* includeHandler = new CustomIncludeHandler;
	IDxcIncludeHandler* includeHandler;
	m_Utils->CreateDefaultIncludeHandler(&includeHandler);

	XWString entryPointW = String2WString(entryPoint);
	TArray<LPCWSTR> preArgs = {
		L"-I", s_ShaderRoot.c_str(),
		L"-spirv"
	};
	m_Utils->BuildArguments(hlslFileW, entryPointW.c_str(), shaderModel, preArgs.Data(), preArgs.Size(), dxcDefines.Data(), dxcDefines.Size(), &args);

	DxcBuffer buffer;
	buffer.Encoding = DXC_CP_ACP;
	buffer.Ptr = sourceBlob->GetBufferPointer();
	buffer.Size = sourceBlob->GetBufferSize();

	IDxcResult* result{ nullptr };
	r = m_Compiler->Compile(
		&buffer,
		args->GetArguments(),
		args->GetCount(),
		includeHandler,
		IID_PPV_ARGS(&result)
	);

	if (SUCCEEDED(r)) {
		result->GetStatus(&r);
	}

	// Output error if compilation failed
	if (FAILED(r) && (result)) {
		IDxcBlobEncoding* errorBlob;
		r = result->GetErrorBuffer(&errorBlob);
		if (SUCCEEDED(r) && errorBlob) {
			LOG_ERROR("Shader(%s, %s) complilation failed: %s", hlslFile.c_str(), entryPoint.c_str(), (const char*)errorBlob->GetBufferPointer());
			return false;
		}
	}

	// Get compilation result
	IDxcBlob* code;
	result->GetResult(&code);

	XString outFile = outFilePath.string();
	File::WFile fout(outFile.c_str(), File::EFileMode::Binary | File::EFileMode::Write);
	if (!fout.is_open()) {
		LOG_ERROR("[HLSL2Spv] write file failed: %s\n", outFile.c_str());
		return false;
	}
	fout.write((char*)code->GetBufferPointer(), code->GetBufferSize());
	fout.close();
	DX_RELEASE(sourceBlob);
	DX_RELEASE(args);
	DX_RELEASE(result);
	DX_RELEASE(code);
	return true;
}
