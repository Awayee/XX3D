#include "Render/Public/GlobalShader.h"
#include "Core/Public/File.h"
#include "System/Public/EngineConfig.h"
#include "HLSLCompiler/HLSLCompiler.h"

namespace Render {

	static TUniquePtr<HLSLCompiler::HLSLCompilerBase> s_HLSLCompiler;

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
		auto rhiType = Engine::ConfigManager::GetData().RHIType;
		if(Engine::ERHIType::Vulkan == rhiType) {
			s_HLSLCompiler.Reset(new HLSLCompiler::SPVCompiler());
		}
		else if (Engine::ERHIType::D3D12 == rhiType) {
			s_HLSLCompiler.Reset(new HLSLCompiler::DXSCompiler());
		}
		else {
			CHECK(0);
		}
	}
}
