#pragma once
#include "Window/Public/ImGuiImpl.h"
#include "Window/Public/InputEnum.h"
#include "Objects/Public/Engine.h"

namespace Editor {
	typedef uint32 WidgetID;

	class WidgetBase {
		friend class EditorUIMgr;
	protected:
		WidgetID m_ID {0};
		virtual void Display() = 0;
	public:
		virtual ~WidgetBase() = default;
	};
}
