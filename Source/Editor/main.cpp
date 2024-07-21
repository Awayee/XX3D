#include "Core/Public/Log.h"
#include "Context/Public/Editor.h"
#include "Engine/Public/Engine.h"
#include "SPVCompiler/SPVCompiler.h"

int main() {
	//compile shaders
	{
		SPVCompiler compiler;
		bool res = compiler.CompileHLSL("GBuffer.hlsl");
		res &= compiler.CompileHLSL("DeferredLightingPBR.hlsl");
	}
	//Run Editor
	{
		LOG_INFO("Editor Mode");
		Editor::XXEditor editor;
		editor.Run();
	}
	//system("pause");
	return 0;
}