#pragma once
#include "EditorUI/Public/EditorWindow.h"

namespace Editor {
	class WndLevelHierarchy : public EditorWndBase {
	public:
		WndLevelHierarchy();
		~WndLevelHierarchy() override;
	private:
		uint32 m_ContextMenuIdx{ INVALID_INDEX };
		uint32 m_RenamingIdx{ INVALID_INDEX };
		TStaticArray<char, NAME_SIZE> m_ActorName{};
		void WndContent() override;
	};
}
