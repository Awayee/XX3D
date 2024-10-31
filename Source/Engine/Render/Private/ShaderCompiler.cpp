#include "Render/Public/ShaderCompiler.h"
#include "Core/Public/File.h"
#include "Core/Public/Container.h"
#include "Core/Public/TUniquePtr.h"
#include "System/Public/Configuration.h"
#include <combaseapi.h>
#include <dxcapi.h>

#define SHADER_SPV_EXTENSION ".spv"
#define SHADER_DXS_EXTENSION ".dxc"
#define SHADER_MODEL_VS L"vs_6_0"
#define SHADER_MODEL_PS L"ps_6_0"
#define SHADER_MODEL_CS L"cs_6_0"

namespace {
	inline const wchar_t* GetShaderModule(EShaderStageFlags stage) {
		switch (stage) {
		case EShaderStageFlags::Vertex:
			return SHADER_MODEL_VS;
		case EShaderStageFlags::Pixel:
			return SHADER_MODEL_PS;
		case EShaderStageFlags::Compute:
			return SHADER_MODEL_CS;
		default:
			return L"";
		}
	}
	
	inline XString FixShaderFileName(const XString& hlslFile) {
		XString result;
		if (auto extIndex = hlslFile.rfind(".hlsl"); extIndex != hlslFile.npos) {
			result = hlslFile.substr(0, extIndex);
		}
		else {
			result = hlslFile;
		}
		std::replace(result.begin(), result.end(), '/', '_');
		return result;
	}

	inline XString GetShaderFullPath(const XString& shaderFile) {
		return Engine::EngineConfig::Instance().GetShaderDir().append(shaderFile).string();
	}

	static DxcCreateInstanceProc GetDXCCreateInstanceProc() {
		static DxcCreateInstanceProc s_Proc{ nullptr };
		if(!s_Proc) {
			HMODULE dxcModule = ::LoadLibrary("dxcompiler.dll");
			if (dxcModule == nullptr) {
				LOG_ERROR("Failed to load dxcompiler.dll");
				return nullptr;
			}
			s_Proc = (DxcCreateInstanceProc)GetProcAddress(dxcModule, "DxcCreateInstance");
		}
		return s_Proc;
	};

	static DxcCreateInstanceProc GetDXILCreateInstanceProc() {
		// load DXIL CreateInstance func
		static DxcCreateInstanceProc s_Proc{ nullptr };
		if(!s_Proc) {
			HMODULE dxilModule = ::LoadLibrary("dxil.dll");
			if (dxilModule == nullptr) {
				LOG_WARNING("Failed to load dxil.dll");
				return nullptr;
			}
			s_Proc = (DxcCreateInstanceProc)GetProcAddress(dxilModule, "DxcCreateInstance");
		}
		return s_Proc;
	}

	template<typename T>
	struct TDXDeleter { void operator()(T* ptr) { ptr->Release(); } };

	template<typename T>
	using TDXCPtr = TUniquePtr<T, TDXDeleter<T>>;

	class IncludeHandler: public IDxcIncludeHandler {
	public:
		IncludeHandler(IDxcUtils* utils) : m_Utils(utils) {}
		~IncludeHandler() = default;
		HRESULT STDMETHODCALLTYPE LoadSource(
			_In_z_ LPCWSTR pFilename,                                 // Candidate filename.
			_COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource  // Resultant source object for included file, nullptr if not found.
		) override {
			XWString fullPath = Engine::EngineConfig::Instance().GetShaderDir().append(pFilename);
			uint32_t codePage = DXC_CP_ACP;
			auto& code = m_Codes.EmplaceBack();
			HRESULT r = m_Utils->LoadFile(fullPath.c_str(), &codePage, code.Address());
			if(SUCCEEDED(r)) {
				*ppIncludeSource = code.Get();
			}
			else {
				*ppIncludeSource = nullptr;
			}
			return r;
		}
		HRESULT QueryInterface(const IID& riid, void** ppvObject) override {
			if (riid == __uuidof(IUnknown) || riid == __uuidof(IDxcIncludeHandler)) {
				*ppvObject = static_cast<IDxcIncludeHandler*>(this);
				AddRef();
				return S_OK;
			}
			*ppvObject = nullptr;
			return E_NOINTERFACE;
		}
		ULONG AddRef() override {
			return InterlockedIncrement(&m_RefCount);
		}
		ULONG Release() override {
			ULONG refCount = InterlockedDecrement(&m_RefCount);
			if (refCount == 0) {
				delete this;
			}
			return refCount;
		}
	private:
		ULONG m_RefCount;
		IDxcUtils* m_Utils;
		TArray<TDXCPtr<IDxcBlobEncoding>> m_Codes;
	};

	class DXCompiler {
	public:
		~DXCompiler() = default;
		DXCompiler() {
			DxcCreateInstanceProc dxcCreateInstance = GetDXCCreateInstanceProc();
			HRESULT r;
			// Initialize DXC library
			r = dxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(m_Library.Address()));
			if (FAILED(r)) {
				LOG_WARNING("[DXCompiler] Could not init DXC Library");
				return;
			}
			// Initialize DXC compiler
			r = dxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(m_Compiler.Address()));
			if (FAILED(r)) {
				LOG_WARNING("[DXCompiler] Could not init DXC Compiler");
				return;
			}

			// Initialize DXC utility
			r = dxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(m_Utils.Address()));
			if (FAILED(r)) {
				LOG_WARNING("[DXCompiler] Could not init DXC Utiliy");
				return;
			}

			const auto rhiType = Engine::ProjectConfig::Instance().RHIType;
			if(Engine::ERHIType::D3D12 == rhiType) {
				DxcCreateInstanceProc dxilCreateInstance = GetDXILCreateInstanceProc();
				r = dxilCreateInstance(CLSID_DxcValidator, IID_PPV_ARGS(m_Validator.Address()));
				if (FAILED(r)) {
					LOG_WARNING("Failed to create validator!");
					return;
				}
			}
			else if (Engine::ERHIType::Vulkan == rhiType) {
				m_PreArgs.PushBack(L"-spirv");
			}
		}
		DXCompiler(const char* code, uint32 byteSize, const XString& entryPoint, EShaderStageFlags stage, TConstArrayView<ShaderCompiler::Macro> macros): DXCompiler() {
			DxcBuffer buffer;
			buffer.Encoding = CP_UTF8;
			buffer.Ptr = code;
			buffer.Size = byteSize;
			m_CompileSucceed = Compile(buffer, entryPoint, stage, macros);
		}
		DXCompiler(const XString& hlslFile, const XString& entryPoint, EShaderStageFlags stage, TConstArrayView<ShaderCompiler::Macro> macros) : DXCompiler() {
			XWString hlslFileW = Engine::EngineConfig::Instance().GetShaderDir().append(hlslFile).wstring();
			// Load the HLSL text shader from disk
			uint32_t codePage = DXC_CP_ACP;
			TDXCPtr<IDxcBlobEncoding> srcCode;
			HRESULT r = m_Utils->LoadFile(hlslFileW.c_str(), &codePage, srcCode.Address());
			if (FAILED(r)) {
				LOG_ERROR("[HLSLCompiler] Could not load shader file: %s", hlslFile.c_str());
				return;
			}

			DxcBuffer buffer;
			buffer.Encoding = DXC_CP_ACP;
			buffer.Ptr = srcCode->GetBufferPointer();
			buffer.Size = srcCode->GetBufferSize();
			m_CompileSucceed = Compile(buffer, entryPoint, stage, macros);
		}

		bool GetResultCode(TArray<int8>& outCode) {
			TDXCPtr<IDxcBlob> blob;
			HRESULT r = m_Result->GetResult(blob.Address());
			if(FAILED(r)) {
				return false;
			}
			if(!SignCode(blob)) {
				return false;
			}
			outCode.Resize((uint32)blob->GetBufferSize());
			memcpy(outCode.Data(), blob->GetBufferPointer(), blob->GetBufferSize());
			return true;
		}

		bool SaveFile(const XString& file) {
			if(!m_CompileSucceed) {
				return false;
			}
			TDXCPtr<IDxcBlob> blob;
			HRESULT r = m_Result->GetResult(blob.Address());
			if(FAILED(r)) {
				return false;
			}
			if(!SignCode(blob)) {
				return false;
			}
			File::FPath fullPath = Engine::EngineConfig::Instance().GetCompiledShaderDir().append(file).string();
			{
				File::FPath outputDirPath = fullPath.parent_path();
				if (!File::Exist(outputDirPath)) {
					File::MakeDir(outputDirPath);
				}
			}
			if(File::WriteFile fOut(fullPath.string(), true); fOut.IsOpen()) {
				fOut.Write(blob->GetBufferPointer(), (uint32)blob->GetBufferSize());
				return true;
			}
			return false;
		}

	private:
		struct DxilMinimalHeader {
			UINT32 FourCC;
			UINT32 HashDigest[4];
		};
		TDXCPtr<IDxcLibrary> m_Library;
		TDXCPtr<IDxcCompiler3> m_Compiler;
		TDXCPtr<IDxcUtils> m_Utils;
		TDXCPtr<IDxcValidator> m_Validator;
		TDXCPtr<IDxcResult> m_Result;
		TArray<LPCWSTR> m_PreArgs;
		bool m_CompileSucceed;

		bool Compile(const DxcBuffer& codeBuffer, const XString& entryPoint, EShaderStageFlags stage, TConstArrayView<ShaderCompiler::Macro> macros) {
			HRESULT r;
			// parse macros
			TArray<XWString> stringCache; stringCache.Reserve(macros.Size() * 2);
			TArray<DxcDefine> dxcDefines; dxcDefines.Reserve(macros.Size());
			for (const auto& define : macros) {
				if (!define.Name.empty()) {
					auto& dxDefine = dxcDefines.EmplaceBack();
					stringCache.PushBack(String2WString(define.Name));
					dxDefine.Name = stringCache.Back().c_str();
					if (define.Value.empty()) {
						dxDefine.Value = nullptr;
					}
					else {
						stringCache.PushBack(String2WString(define.Value));
						dxDefine.Value = stringCache.Back().c_str();
					}
				}
			}

			// build args
			const XWString entryPointW = String2WString(entryPoint);
			const wchar_t* shaderModel = GetShaderModule(stage);
			TDXCPtr<IDxcCompilerArgs> args;
			m_Utils->BuildArguments(nullptr, entryPointW.c_str(), shaderModel, m_PreArgs.Data(), m_PreArgs.Size(), dxcDefines.Data(), dxcDefines.Size(), args.Address());

			// include handler
			TDXCPtr<IncludeHandler> includeHandler(new IncludeHandler(m_Utils.Get()));

			r = m_Compiler->Compile(&codeBuffer, args->GetArguments(), args->GetCount(), includeHandler, IID_PPV_ARGS(m_Result.Address()));
			if (SUCCEEDED(r)) {
				m_Result->GetStatus(&r);
			}

			// Output error if compilation failed
			if (FAILED(r) && (m_Result)) {
				IDxcBlobEncoding* errorBlob;
				r = m_Result->GetErrorBuffer(&errorBlob);
				if (SUCCEEDED(r) && errorBlob) {
					LOG_ERROR("[Shader Compile Error] %s", (const char*)errorBlob->GetBufferPointer());
				}
				return false;
			}
			return true;
		}

		bool IsCodeSigned(IDxcBlob* blob) {
			if (blob->GetBufferSize() < sizeof(DxilMinimalHeader)) {
				return false;
			}
			DxilMinimalHeader* header = (DxilMinimalHeader*)blob->GetBufferPointer();
			bool has_digest = false;
			has_digest |= header->HashDigest[0] != 0x0;
			has_digest |= header->HashDigest[1] != 0x0;
			has_digest |= header->HashDigest[2] != 0x0;
			has_digest |= header->HashDigest[3] != 0x0;
			return has_digest;
		}
		bool SignCode(IDxcBlob* blob) {
			// =============== sign code for d3d12 ===============
			if (!m_Validator) {
				return true;
			}
			if (IsCodeSigned(blob)) {
				return true;
			}
			TDXCPtr<IDxcBlobEncoding> containerBlob;
			m_Library->CreateBlobWithEncodingFromPinned(blob->GetBufferPointer(), blob->GetBufferSize(), 0 /* binary, no code page */, containerBlob.Address());

			// Query validation version info
			{
				TDXCPtr<IDxcVersionInfo> versionInfo;
				if (FAILED(m_Validator->QueryInterface(IID_PPV_ARGS(versionInfo.Address())))) {
					LOG_WARNING("Failed to query version info interface");
					return false;
				}

				UINT32 major = 0;
				UINT32 minor = 0;
				versionInfo->GetVersion(&major, &minor);
				LOG_DEBUG("Validator version: %u.%u", major, minor);
			}

			TDXCPtr<IDxcOperationResult> result;
			if (FAILED(m_Validator->Validate(containerBlob.Get(), DxcValidatorFlags_InPlaceEdit /* avoid extra copy owned by dxil.dll */, result.Address()))) {
				LOG_WARNING("Failed to validate dxil container");
				return false;
			}

			HRESULT validateStatus;
			if (FAILED(result->GetStatus(&validateStatus))) {
				LOG_WARNING("Failed to get dxil validate status");
				return false;
			}

			if (FAILED(validateStatus)) {
				LOG_WARNING("The dxil container failed validation");
				std::string errorString;
				TDXCPtr<IDxcBlobEncoding> printBlob, printBlobUtf8;
				result->GetErrorBuffer(printBlob.Address());

				m_Library->GetBlobAsUtf8(printBlob.Get(), printBlobUtf8.Address());
				if (printBlobUtf8) {
					errorString = reinterpret_cast<const char*>(printBlobUtf8->GetBufferPointer());
				}
				LOG_WARNING("Error: %s", errorString.c_str());
				return false;
			}

			if (!IsCodeSigned(blob)) {
				LOG_WARNING("Signing failed!");
				return false;
			}
			return true;

		}
	};
}

namespace ShaderCompiler {

	XString GetCompileOutputFile(const XString& hlslFile, const XString& entryPoint, uint32 permutationID) {
		if(Engine::ProjectConfig::Instance().RHIType == Engine::ERHIType::D3D12) {
			return FixShaderFileName(hlslFile).append(entryPoint).append(ToString(permutationID)).append(SHADER_DXS_EXTENSION);
		}
		return FixShaderFileName(hlslFile).append(entryPoint).append(ToString(permutationID)).append(SHADER_SPV_EXTENSION);
	}

	bool LoadCompiledShader(const XString& compiledFile, TArray<int8>& outCode) {
		const XString fullPath = Engine::EngineConfig::Instance().GetCompiledShaderDir().append(compiledFile).string();
		if (File::ReadFileWithSize f(fullPath.c_str(), true); f.IsOpen()) {
			outCode.Resize(f.ByteSize());
			f.Read(outCode.Data(), f.ByteSize());
			return true;
		}
		return false;
	}

	bool LoadSourceShader(const XString& shaderFile, XString& outCode) {
		const XString fullPath = Engine::EngineConfig::Instance().GetShaderDir().append(shaderFile).string();
		if (File::ReadFileWithSize f(fullPath, false); f.IsOpen()) {
			outCode.resize(f.ByteSize());
			f.Read(outCode.data(), f.ByteSize());
			return true;
		}
		return false;
	}

	bool CompileSourceFileToFile(const XString& hlslFile, const XString& entryPoint, EShaderStageFlags stage, TConstArrayView<Macro> macros, uint32 permutationID) {
		XString outFile = GetCompileOutputFile(hlslFile, entryPoint, permutationID);
		DXCompiler compiler(hlslFile, entryPoint, stage, macros);
		return compiler.SaveFile(outFile);
	}

	bool CompileCodeToFile(const XString& hlslCode, const XString& entryPoint, EShaderStageFlags stage, TConstArrayView<Macro> macros, const XString& outFile) {
		DXCompiler compiler(hlslCode.data(), (uint32)hlslCode.size(), entryPoint, stage, macros);
		return compiler.SaveFile(outFile);
	}

	bool CompileCodeToBytes(const XString& hlslCode, const XString& entryPoint, EShaderStageFlags stage, TConstArrayView<Macro> macros, TArray<int8>& outBytes) {
		DXCompiler compiler(hlslCode.data(), (uint32)hlslCode.size(), entryPoint, stage, macros);
		return compiler.GetResultCode(outBytes);
	}
}
