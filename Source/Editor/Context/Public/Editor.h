#pragma once
#include "Core/Public/TUniquePtr.h"
#include "Engine/Public/Engine.h"

namespace Editor {
	class UIController;
	class XXEditor: public Engine::XXEngine {
	public:
		XXEditor();
		~XXEditor() override;
		TUniquePtr<UIController> m_UIController;
	protected:
		void Update() override;
	};
}
