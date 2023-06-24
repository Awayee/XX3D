#pragma once
#include "EditorUI/Public/EditorWindow.h"

namespace Editor {
	class WndLevelHierarchical : public EditorWindowBase {
		uint32 m_SelectIdx=UINT32_MAX;
	private:
		void Update() override;
		void Display() override;
	public:
		WndLevelHierarchical();
		~WndLevelHierarchical() override;
	};
}
