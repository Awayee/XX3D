#include "Core/Public/Log.h"
#include "Context/Public/Editor.h"
#include "Engine/Public/Engine.h"
#include "HLSLCompiler/HLSLCompiler.h"

int main() {
	//clear all compiled shader cache
	{
		HLSLCompiler::ClearShaderCompileCache();
	}
	//Run Editor
	{
		LOG_INFO("Editor Mode");
		Editor::XXEditor::PreInitialize();
		Editor::XXEditor editor;
		editor.Run();
	}
	//system("pause");
	return 0;
}