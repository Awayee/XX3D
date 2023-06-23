#pragma once
#include "Objects/Public/Engine.h"

namespace Editor {
	class XXEditor {
	private:
		Engine::XXEngine* m_Engine;

	public:
		XXEditor(Engine::XXEngine* engine);
		~XXEditor();
		void EditorRun();
	};
}
