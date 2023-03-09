#include "Core/Public/macro.h"
#include "Context/Public/Editor.h"
#include "Objects/Public/Engine.h"

int main() {
	// Run Editor
	{
		LOG("Editor Mode");
		Engine::XXEngine engine{};
		Editor::XXEditor editor(&engine);
		editor.EditorRun();
	}

	//system("pause");
	return 0;
}