#include "Core/Public/Log.h"
#include "Context/Public/Editor.h"
#include "Engine/Public/Engine.h"
int main() {
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