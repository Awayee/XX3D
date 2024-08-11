#pragma once
#include "Core/Public/File.h"
#include "Core/Public/String.h"
#include "Core/Public/TVector.h"
#include <combaseapi.h>
#include <dxcapi.h>

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

class SPVCompiler {
public:
	SPVCompiler();
	~SPVCompiler();
	// hlslFile and outFile is relative path
	static XString GetShaderOutputFile(const XString& hlslFile, const XString& entryPoint);
	// Clear all compiled shader cache, for recompiling all.
	static void ClearCompiledCache();
	bool CompileHLSL(const XString& hlslFile, const XString& entryPoint, ESPVShaderStage stage, TConstArrayView<SPVDefine> defines);
private:
	IDxcLibrary* m_Library{ nullptr };
	IDxcCompiler3* m_Compiler{ nullptr };
	IDxcUtils* m_Utils{ nullptr };
};
