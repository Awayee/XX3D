#pragma once
#include "EditorUI/Public/EditorWindow.h"

namespace Editor {
	class WndLevelHierarchy : public EditorWndBase {
	private:
		enum {INVALID_INDEX = UINT32_MAX};
		uint32 m_SelectIdx = INVALID_INDEX;
		uint32 m_PressedIdx = INVALID_INDEX;
		bool m_Pressed = false;
	private:
		void Update() override;
		void WndContent() override;
	public:
		WndLevelHierarchy();
		~WndLevelHierarchy() override;
	};
}
