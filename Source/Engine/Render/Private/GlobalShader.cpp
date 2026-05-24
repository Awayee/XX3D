#include "Render/Public/GlobalShader.h"

namespace Render {
	GlobalShader::GlobalShader(XStringView shaderFile, XStringView entry, EShaderStageFlags type, TConstArrayView<ShaderCompiler::Macro> macros, uint32 permutationID) {
		TArray<int8> codeData;
		const XString outputFile = ShaderCompiler::GetCompileOutputFile(shaderFile, entry, permutationID);
		if (!ShaderCompiler::LoadCompiledShader(outputFile, codeData)) {
			for (const auto& macro : macros) {
				LOG_DEBUG("    #define %s %s", macro.Name.c_str(), macro.Value.c_str());
			}
			if (ShaderCompiler::CompileSourceFileToFile(shaderFile, entry, type, macros, permutationID)) {
				LOG_DEBUG("Shader compiled: %s, %s, %s", XString{shaderFile}.c_str(), XString{entry}.c_str(), outputFile.c_str());
				if (!ShaderCompiler::LoadCompiledShader(outputFile, codeData)) {
					LOG_FATAL("Could not load shader file: %s", outputFile.c_str());
				}
			}
			else {
				LOG_FATAL("Failed to compile shader: %s, %s", XString{shaderFile}.c_str(), XString{entry}.c_str());
			}
		}
		m_RHIShader = RHI::Instance()->CreateShader(type, XStringView{codeData.Data(), codeData.Size()}, entry, this);
		m_RHIShader->SetName(outputFile.c_str());
	}

	RHIShader* GlobalShader::GetRHI() {
		return m_RHIShader.Get();
	}
}
