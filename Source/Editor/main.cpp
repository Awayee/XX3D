#include "Core/Public/Log.h"
#include "Context/Public/Editor.h"
#include "Engine/Public/Engine.h"
#include "SPVCompiler/SPVCompiler.h"

int main() {
	//clear all compiled shader cache
	{
		SPVCompiler::ClearCompiledCache();
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