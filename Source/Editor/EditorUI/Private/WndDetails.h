#pragma once
#include "EditorUI/Public/EditorWindow.h"

namespace Editor {
	class WndDetails : public EditorWndBase {
	private:
		void Update() override;
		void WndContent() override;
	public:
		WndDetails();
	};
}
