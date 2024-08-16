#include "Render/Public/GlobalShader.h"
#include "Core/Public/File.h"
#include "SPVCompiler/SPVCompiler.h"

namespace Render {

	inline bool LoadShaderCode(const XString& shaderFile, TArray<int8>& outCode) {
		File::FPath shaderFilePath{ SHADER_PATH };
		shaderFilePath.append(shaderFile);
		const XString shaderFileAbs = shaderFilePath.string();
		File::RFile f(shaderFileAbs, File::EFileMode::AtEnd | File::EFileMode::Binary);
		if (!f.is_open()) {
			return false;
		}

		uint32 fileSize = (uint32)f.tellg();
		outCode.Resize(fileSize);
		f.seekg(0);
		f.read(outCode.Data(), fileSize);
		f.close();
		return true;
	}

	inline ESPVShaderStage ToSPVShaderStage(EShaderStageFlagBit type) {
		switch(type) {
		case EShaderStageFlagBit::SHADER_STAGE_VERTEX_BIT: return ESPVShaderStage::Vertex;
		case EShaderStageFlagBit::SHADER_STAGE_PIXEL_BIT: return ESPVShaderStage::Pixel;
		case EShaderStageFlagBit::SHADER_STAGE_COMPUTE_BIT: return ESPVShaderStage::Compute;
		default: return ESPVShaderStage::Invalid;
		}
	}
	GlobalShader::GlobalShader(const XString& shaderFile, const XString& entry, EShaderStageFlagBit type) {
		const XString spvFile = SPVCompiler::GetShaderOutputFile(shaderFile, entry);
		TArray<int8> codeData;
		if(!LoadShaderCode(spvFile, codeData)) {
			if(SPVCompiler{}.CompileHLSL(shaderFile, entry, ToSPVShaderStage(type), {})) {
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
	}

	GlobalShaderMap::~GlobalShaderMap() {
	}
}
