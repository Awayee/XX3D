#include "Core/Public/Defines.h"
#include "Context/Public/Editor.h"
#include "Objects/Public/Engine.h"
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
		PRINT("Editor Mode");
		Engine::XXEngine engine{};
		Editor::XXEditor editor(&engine);
		editor.EditorRun();
	}

	//system("pause");
	return 0;
}