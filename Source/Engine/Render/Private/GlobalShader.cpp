#include "Render/Public/GlobalShader.h"
#include "Core/Public/File.h"
#include "System/Public/Configuration.h"
#include "HLSLCompiler/HLSLCompiler.h"

#define SHADER_SPV_EXTENSION ".spv"
#define SHADER_DXS_EXTENSION ".dxc"

namespace Render {

	static TUniquePtr<HLSLCompilerBase> s_HLSLCompiler;

	inline bool LoadShaderCode(const XString& shaderFile, TArray<int8>& outCode) {
		const File::FPath shaderFilePath{ shaderFile };
		const XString shaderFileAbs = shaderFilePath.string();
		if(File::ReadFileWithSize f(shaderFileAbs.c_str(), true); f.IsOpen()) {
			outCode.Resize(f.ByteSize());
			f.Read(outCode.Data(), f.ByteSize());
			return true;
		}
		return false;
	}

	inline HLSLCompiler::ESPVShaderStage ToSPVShaderStage(EShaderStageFlags type) {
		switch(type) {
		case EShaderStageFlags::Vertex: return HLSLCompiler::ESPVShaderStage::Vertex;
		case EShaderStageFlags::Pixel: return HLSLCompiler::ESPVShaderStage::Pixel;
		case EShaderStageFlags::Compute: return HLSLCompiler::ESPVShaderStage::Compute;
		default: return HLSLCompiler::ESPVShaderStage::Invalid;
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

	class SPVCompiler : public HLSLCompilerBase {
	public:
		XString GetCompileOutputFile(const XString& hlslFile, const XString& entryPoint, uint32 permutationID) override {
			XString resultFile = Engine::EngineConfig::Instance().GetCompiledShaderDir().append(FixShaderFileName(hlslFile)).string();
			resultFile.append(entryPoint).append(ToString(permutationID)).append(SHADER_SPV_EXTENSION);
			return resultFile;
		}
		bool Compile(const XString& hlslFile, const XString& entryPoint, ShaderStage stage, TConstArrayView<ShaderDefine> defines, uint32 permutationID) override {
			XString outputFile = GetCompileOutputFile(hlslFile, entryPoint, permutationID);
			return HLSLCompiler::CompileHLSLToSPV(GetShaderFullPath(hlslFile), entryPoint, stage, defines, outputFile);
		}
	};

	class DXSCompiler : public HLSLCompilerBase {
	public:
		XString GetCompileOutputFile(const XString& hlslFile, const XString& entryPoint, uint32 permutationID) override {
			XString resultFile = Engine::EngineConfig::Instance().GetCompiledShaderDir().append(FixShaderFileName(hlslFile)).string();
			resultFile.append(entryPoint).append(ToString(permutationID)).append(SHADER_DXS_EXTENSION);
			return resultFile;
		}
		bool Compile(const XString& hlslFile, const XString& entryPoint, ShaderStage stage, TConstArrayView<ShaderDefine> defines, uint32 permutationID) override{
			XString outputFile = GetCompileOutputFile(hlslFile, entryPoint, permutationID);
			return HLSLCompiler::CompileHLSLWithSign(GetShaderFullPath(hlslFile), entryPoint, stage, defines, outputFile);
		}
	};

	GlobalShader::GlobalShader(const XString& shaderFile, const XString& entry, EShaderStageFlags type, TConstArrayView<ShaderDefine> defines, uint32 permutationID) {
		TArray<int8> codeData;
		const XString outputFile = s_HLSLCompiler->GetCompileOutputFile(shaderFile, entry, permutationID);
		if(!LoadShaderCode(outputFile, codeData)) {
			if(s_HLSLCompiler->Compile(shaderFile, entry, ToSPVShaderStage(type), defines, permutationID)) {
				for(const auto& define: defines) {
					LOG_DEBUG("    #define %s %s", define.Name.c_str(), define.Value.c_str());
				}
				LOG_DEBUG("Shader compiled: %s, %s, %s", shaderFile.c_str(), entry.c_str(), outputFile.c_str());
				if (!LoadShaderCode(outputFile, codeData)) {
					LOG_ERROR("Could not load shader file: %s", outputFile.c_str());
				}
			}
			else {
				LOG_ERROR("Failed to compile shader: %s, %s", shaderFile.c_str(), entry.c_str());
			}
		}
		m_RHIShader = RHI::Instance()->CreateShader(type, codeData.Data(), codeData.Size(), entry);
	}

	RHIShader* GlobalShader::GetRHI() {
		return m_RHIShader.Get();
	}

	GlobalShaderMap::GlobalShaderMap() {
		// initialize shader compiler
		auto rhiType = Engine::ProjectConfig::Instance().RHIType;
		if(Engine::ERHIType::Vulkan == rhiType) {
			s_HLSLCompiler.Reset(new SPVCompiler());
		}
		else if (Engine::ERHIType::D3D12 == rhiType) {
			s_HLSLCompiler.Reset(new DXSCompiler());
		}
		else {
			CHECK(0);
		}
	}
}
