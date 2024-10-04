#pragma once
#include "EditorUI/Public/EditorWindow.h"

namespace Editor {
	class WndDetails : public EditorWndBase {
	public:
		WndDetails();
	private:
		bool m_IsAddingComponent{ false };
		uint32 m_EditCompIdx{ INVALID_INDEX };
		void Update() override;
		void WndContent() override;
	};
}
