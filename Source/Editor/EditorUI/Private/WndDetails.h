#pragma once
#include "EditorUI/Public/EditorWindow.h"

namespace Editor {
	class WndDetails : public EditorWindowBase {
	private:
		void Update() override;
		void Display() override;
	public:
		WndDetails();
	};
}
