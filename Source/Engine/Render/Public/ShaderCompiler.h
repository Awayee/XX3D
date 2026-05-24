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
	XString GetCompileOutputFile(XStringView hlslFile, XStringView entryPoint, uint32 permutationID);

	// Load compiled shader code
	bool LoadCompiledShader(XStringView compiledFile, TArray<int8>& outCode);

	// Load .hlsl shader code
	bool LoadSourceShader(XStringView shaderFile, XString& outCode);

	// Compile .hlsl file
	bool CompileSourceFileToFile(XStringView hlslFile, XStringView entryPoint, EShaderStageFlags stage, TConstArrayView<Macro> macros, uint32 permutationID);

	// Compile code
	bool CompileCodeToFile(XStringView hlslCode, XStringView entryPoint, EShaderStageFlags stage, TConstArrayView<Macro> macros, XString& outFile);

	// Compile code to byte code.
	bool CompileCodeToBytes(XStringView hslCode, XStringView entryPoint, EShaderStageFlags stage, TConstArrayView<Macro> macros, TArray<int8>& outBytes);

	// Save compiled code
	bool SaveCompiledBytesToFile(const TArray<int8>& bytes, XStringView compiledFile);
}
