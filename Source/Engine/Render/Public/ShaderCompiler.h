#pragma once
#include "Core/Public/File.h"
#include "Core/Public/String.h"
#include "Core/Public/TArray.h"
#include "Core/Public/TUniquePtr.h"
#include "RHI/Public/RHIEnum.h"

namespace ShaderCompiler {
	struct Macro {
		XString Name;
		XString Value;
	};
	// Get shader output file by .hlsl file, entry func, and macros
	XString GetCompileOutputFile(const XString& hlslFile, const XString& entryPoint, uint32 permutationID);

	// Load compiled shader code
	bool LoadCompiledShader(const XString& compiledFile, TArray<int8>& outCode);

	// Load .hlsl shader code
	bool LoadSourceShader(const XString& shaderFile, XString& outCode);

	// Compile .hlsl file
	bool CompileSourceFileToFile(const XString& hlslFile, const XString& entryPoint, EShaderStageFlags stage, TConstArrayView<Macro> macros, uint32 permutationID);

	// Compile code
	bool CompileCodeToFile(const XString& hlslCode, const XString& entryPoint, EShaderStageFlags stage, TConstArrayView<Macro> macros, const XString& outFile);

	// Compile code to byte code.
	bool CompileCodeToBytes(const XString& hslCode, const XString& entryPoint, EShaderStageFlags stage, TConstArrayView<Macro> macros, TArray<int8>& outBytes);
}
