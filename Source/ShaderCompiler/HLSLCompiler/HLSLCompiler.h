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
	// Clear all compiled shader cache, for recompiling all.
	void ClearShaderCompileCache();

	class HLSLCompilerBase {
	public:
		virtual ~HLSLCompilerBase() = default;
		virtual XString GetCompileOutputFile(const XString& hlslFile, const XString& entryPoint, uint32 permutationID) = 0;
		virtual bool Compile(const XString& hlslFile, const XString& entryPoint, ESPVShaderStage stage, TConstArrayView<SPVDefine> defines, uint32 permutationID) = 0;
	};

	class SPVCompiler: public HLSLCompilerBase {
	public:
		XString GetCompileOutputFile(const XString& hlslFile, const XString& entryPoint, uint32 permutationID) override;
		bool Compile(const XString& hlslFile, const XString& entryPoint, ESPVShaderStage stage, TConstArrayView<SPVDefine> defines, uint32 permutationID) override;
	};

	class DXSCompiler: public HLSLCompilerBase {
	public:
		XString GetCompileOutputFile(const XString& hlslFile, const XString& entryPoint, uint32 permutationID) override;
		bool Compile(const XString& hlslFile, const XString& entryPoint, ESPVShaderStage stage, TConstArrayView<SPVDefine> defines, uint32 permutationID) override;
	};
}
