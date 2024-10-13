#pragma once
#include "Core/Public/File.h"
#include "Core/Public/String.h"
#include "Core/Public/TArray.h"

namespace HLSLCompiler {
	enum class ESPVShaderStage {
		Vertex,
		Pixel,
		Compute,
		Invalid,
	};

	struct SPVDefine {
		XString Name;
		XString Value;
	};

	bool CompileHLSLToSPV(const XString& hlslFile, const XString& entryPoint, ESPVShaderStage stage, TConstArrayView<SPVDefine> defines, const XString& outputFile);

	bool CompileHLSLWithSign(const XString& hlslFile, const XString& entryPoint, ESPVShaderStage stage, TConstArrayView<SPVDefine> defines, const XString& outputFile);
}
