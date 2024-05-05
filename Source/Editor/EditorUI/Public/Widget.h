#pragma once
#include "Core/Public/TypeDefine.h"
#include "Window/Public/InputEnum.h"
#include "Objects/Public/Engine.h"

namespace Editor {
	typedef uint32 WidgetID;

	class WidgetBase {
		friend class EditorUIMgr;
	protected:
		WidgetID m_ID {0};
		bool m_ToDelete{ false };//mark this widget is to delete
		virtual void Update() = 0;
		virtual void Display() = 0;
	public:
		virtual ~WidgetBase() = default;
	};
}
