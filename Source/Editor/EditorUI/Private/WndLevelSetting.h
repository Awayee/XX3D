#pragma once
#include "EditorUI/Public/EditorWindow.h"

namespace Object {
	class Camera;
}

namespace Editor {
	class WndLevelSetting : public EditorWndBase {
	public:
		WndLevelSetting();
		~WndLevelSetting() override;
	private:
		void WndContent() override;
	};
}
