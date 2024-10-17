#pragma once
#include "EditorUI/Public/EditorWindow.h"
#include "Core/Public/Container.h"

namespace ax {
	namespace NodeEditor {
		struct EditorContext;
	}
}

namespace Editor {
	class WndRenderGraph: public EditorWndBase {
	public:
		WndRenderGraph();
		~WndRenderGraph() override = default;
	private:
		uint32 m_UpdateFrame;
		TArray<TPair<int, int>> m_NodePins; // pin id, {inPin, outPin}
		TArray<TPair<int, int>> m_NodeLines; // index of pin
		void WndContent() override;
		void OnOpen() override;
	};
}
