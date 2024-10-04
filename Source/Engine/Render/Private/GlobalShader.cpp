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
	GlobalShader::GlobalShader(const XString& shaderFile, const XString& entry, EShaderStageFlags type) {
		TArray<int8> codeData;
		const XString spvFile = s_HLSLCompiler->GetCompileOutputFile(shaderFile, entry);
		if(!LoadShaderCode(spvFile, codeData)) {
			if(s_HLSLCompiler->Compile(shaderFile, entry, ToSPVShaderStage(type), {})) {
				LOG_DEBUG("Shader compiled: %s, %s, %s", shaderFile.c_str(), entry.c_str(), spvFile.c_str());
				if (!LoadShaderCode(spvFile, codeData)) {
					LOG_ERROR("Could not load shader file: %s", spvFile.c_str());
				}
			}
			else {
				LOG_ERROR("Failed to compile shader: %s, %s", shaderFile.c_str(), entry.c_str());
			}
		}
		m_RHIShader = RHI::Instance()->CreateShader(type, codeData.Data(), codeData.Size(), entry);
	}

	GlobalShader::~GlobalShader() {
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

	GlobalShaderMap::~GlobalShaderMap() {
	}
}
