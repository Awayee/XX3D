#pragma once
#include "Core/Public/SmartPointer.h"
#include "Objects/Public/Engine.h"

namespace Editor {
	class UIController;
	class XXEditor {
	private:
		Engine::XXEngine* m_Engine;
		TUniquePtr<UIController> m_UIController;

	public:
		XXEditor(Engine::XXEngine* engine);
		~XXEditor();
		void EditorRun();
	};
}
