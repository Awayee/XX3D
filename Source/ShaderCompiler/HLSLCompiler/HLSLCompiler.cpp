#include "HLSLCompiler.h"
#include "Core/Public/File.h"
#include "Core/Public/Container.h"
#include "Core/Public/TUniquePtr.h"
#include <combaseapi.h>
#include <dxcapi.h>

#define SHADER_MODEL_VS L"vs_6_0"
#define SHADER_MODEL_PS L"ps_6_0"
#define SHADER_MODEL_CS L"cs_6_0"
#define SHADER_COMPILE_OUTPUT SHADER_PATH##"compiled/"
#define SHADER_SPV_EXTENSION ".spv"
#define SHADER_DXS_EXTENSION ".dxc"

namespace {
	using namespace HLSLCompiler;

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

	inline const wchar_t* GetShaderModule(ESPVShaderStage stage) {
		switch (stage) {
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

	template<typename T>
	struct TDXDeleter { void operator()(T* ptr) { ptr->Release(); } };

	template<typename T>
	using TDXPtr = TUniquePtr<T, TDXDeleter<T>>;



	class DXCompiler {
	public:
		DXCompiler(bool needSignCode) {
			// create library
			HMODULE dxcModule = ::LoadLibrary("dxcompiler.dll");
			if (dxcModule == nullptr) {
				LOG_WARNING("Failed to load dxcompiler.dll");
				return;
			}
			HRESULT r;
			DxcCreateInstanceProc dxcCreateInstance = (DxcCreateInstanceProc)GetProcAddress(dxcModule, "DxcCreateInstance");
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

			if(needSignCode) {
				// load DXIL CreateInstance func
				HMODULE dxilModule = ::LoadLibrary("dxil.dll");
				if (dxilModule == nullptr) {
					LOG_WARNING("Failed to load dxil.dll");
					return;
				}
				DxcCreateInstanceProc dxilCreateInstance = (DxcCreateInstanceProc)GetProcAddress(dxilModule, "DxcCreateInstance");
				r = dxilCreateInstance(CLSID_DxcValidator, IID_PPV_ARGS(m_Validator.Address()));
				if (FAILED(r)) {
					LOG_WARNING("Failed to create validator!");
					return;
				}
			}
		}
		~DXCompiler() = default;
		bool IsCodeSigned(void* code) {
			DxilMinimalHeader* header = (DxilMinimalHeader*)code;
			bool has_digest = false;
			has_digest |= header->HashDigest[0] != 0x0;
			has_digest |= header->HashDigest[1] != 0x0;
			has_digest |= header->HashDigest[2] != 0x0;
			has_digest |= header->HashDigest[3] != 0x0;
			return has_digest;
		}
		bool SignCode(BYTE* byteData, UINT32 byteSize) {
			if(!m_Validator) {
				return true;
			}
			if (byteSize < sizeof(DxilMinimalHeader)) {
				return false;
			}
			if(IsCodeSigned(byteData)) {
				return true;
			}
			TDXPtr<IDxcBlobEncoding> containerBlob;
			m_Library->CreateBlobWithEncodingFromPinned(byteData, byteSize, 0 /* binary, no code page */, containerBlob.Address());

			// Query validation version info
			{
				TDXPtr<IDxcVersionInfo> versionInfo;
				if (FAILED(m_Validator->QueryInterface(IID_PPV_ARGS(versionInfo.Address())))) {
					LOG_WARNING("Failed to query version info interface");
					return false;
				}

				UINT32 major = 0;
				UINT32 minor = 0;
				versionInfo->GetVersion(&major, &minor);
				LOG_DEBUG("Validator version: %u.%u", major, minor);
			}

			TDXPtr<IDxcOperationResult> result;
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
				TDXPtr<IDxcBlobEncoding> printBlob, printBlobUtf8;
				result->GetErrorBuffer(printBlob.Address());

				m_Library->GetBlobAsUtf8(printBlob.Get(), printBlobUtf8.Address());
				if (printBlobUtf8) {
					errorString = reinterpret_cast<const char*>(printBlobUtf8->GetBufferPointer());
				}
				LOG_WARNING("Error: %s", errorString.c_str());
				return false;
			}

			if (!IsCodeSigned(byteData)) {
				LOG_WARNING("Signing failed!");
				return false;
			}
			return true;
		}
		
		bool Compiler(const XString& hlslFile, const XString& entryPoint, ESPVShaderStage stage, TConstArrayView<SPVDefine> defines, TArrayView<LPCWSTR> preArgs, const XString& outputFile) {
			File::FPath inputFile{ SHADER_PATH };
			inputFile.append(hlslFile);
			const wchar_t* hlslFileW = inputFile.c_str();
			// make file directory if not exist
			{
				File::FPath outFilePath{ SHADER_COMPILE_OUTPUT };
				if (!File::Exist(outFilePath)) {
					File::MakeDir(outFilePath);
				}
			}
			// Load the HLSL text shader from disk
			uint32_t codePage = DXC_CP_ACP;
			TDXPtr<IDxcBlobEncoding> sourceBlob;
			HRESULT r = m_Utils->LoadFile(hlslFileW, &codePage, sourceBlob.Address());
			if (FAILED(r)) {
				LOG_ERROR("[HLSLCompiler] Could not load shader file: %s", hlslFile.c_str());
				return false;
			}

			// parse defines
			TArray<TPair<XWString, XWString>> definesW; definesW.Reserve(defines.Size());
			TArray<DxcDefine> dxcDefines; dxcDefines.Reserve(defines.Size());
			for (const auto& define : defines) {
				auto& defineW = definesW.EmplaceBack();
				defineW.first = String2WString(define.Name);
				defineW.second = String2WString(define.Value);
				dxcDefines.PushBack({ defineW.first.c_str(), defineW.second.c_str() });
			}

			// build args
			XWString entryPointW = String2WString(entryPoint);
			const wchar_t* shaderModel = GetShaderModule(stage);
			TDXPtr<IDxcCompilerArgs> args;
			m_Utils->BuildArguments(hlslFileW, entryPointW.c_str(), shaderModel, preArgs.Data(), preArgs.Size(), dxcDefines.Data(), dxcDefines.Size(), args.Address());

			// include handler
			TDXPtr<IDxcIncludeHandler> includeHandler;
			m_Utils->CreateDefaultIncludeHandler(includeHandler.Address());

			DxcBuffer buffer;
			buffer.Encoding = DXC_CP_ACP;
			buffer.Ptr = sourceBlob->GetBufferPointer();
			buffer.Size = sourceBlob->GetBufferSize();
			TDXPtr<IDxcResult> result;
			r = m_Compiler->Compile(&buffer, args->GetArguments(), args->GetCount(), includeHandler, IID_PPV_ARGS(result.Address()));
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
			TDXPtr<IDxcBlob> code;
			result->GetResult(code.Address());

			// Sign and validation
			if(!SignCode((BYTE*)code->GetBufferPointer(), code->GetBufferSize())) {
				LOG_ERROR("[HLSLCompiler] Failed to sign code!");
				return false;
			}

			File::WFile fout(outputFile.c_str(), File::EFileMode::Binary | File::EFileMode::Write);
			if (!fout.is_open()) {
				LOG_ERROR("[HLSLCompiler] Failed to write file: %s", outputFile.c_str());
				return false;
			}
			fout.write((char*)code->GetBufferPointer(), code->GetBufferSize());
			fout.close();
			return true;
		}
	private:
		struct DxilMinimalHeader {
			UINT32 FourCC;
			UINT32 HashDigest[4];
		};
		TDXPtr<IDxcLibrary> m_Library;
		TDXPtr<IDxcCompiler3> m_Compiler;
		TDXPtr<IDxcUtils> m_Utils;
		TDXPtr<IDxcValidator> m_Validator;
	};
}

namespace HLSLCompiler {

	void ClearShaderCompileCache() {
		const File::FPath path{ SHADER_COMPILE_OUTPUT };
		File::RemoveDir(path);
	}

	XString SPVCompiler::GetCompileOutputFile(const XString& hlslFile, const XString& entryPoint) {
		XString resultFile{ SHADER_COMPILE_OUTPUT };
		resultFile.append(FixShaderFileName(hlslFile)).append(entryPoint).append(SHADER_SPV_EXTENSION);
		return resultFile;
	}

	bool SPVCompiler::Compile(const XString& hlslFile, const XString& entryPoint, ESPVShaderStage stage, TConstArrayView<SPVDefine> defines) {
		TArray<LPCWSTR> args{ L"-spirv" };
		XString outputFile = GetCompileOutputFile(hlslFile, entryPoint);
		return DXCompiler{ false }.Compiler(hlslFile, entryPoint, stage, defines, args, outputFile);
	}

	XString DXSCompiler::GetCompileOutputFile(const XString& hlslFile, const XString& entryPoint) {
		XString resultFile{ SHADER_COMPILE_OUTPUT };
		resultFile.append(FixShaderFileName(hlslFile)).append(entryPoint).append(SHADER_DXS_EXTENSION);
		return resultFile;
	}

	bool DXSCompiler::Compile(const XString& hlslFile, const XString& entryPoint, ESPVShaderStage stage, TConstArrayView<SPVDefine> defines) {
		XString outputFile = GetCompileOutputFile(hlslFile, entryPoint);
		return DXCompiler{ true }.Compiler(hlslFile, entryPoint, stage, defines, {}, outputFile);
	}
}