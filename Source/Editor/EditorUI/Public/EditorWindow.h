#pragma once
#include "Widget.h"
#include "Core/Public/Func.h"

namespace Editor {

	class EditorWndBase: public WidgetBase {
		friend class UIMgr;
	protected:
		bool m_Enable{ true };
		const char* m_Name = "Unknown";
		ImGuiWindowFlags m_Flags = ImGuiWindowFlags_None;
	protected:
		virtual void Update() = 0;
		virtual void Display() = 0;
	public:
		EditorWndBase(const char* name, ImGuiWindowFlags flags = ImGuiWindowFlags_None);
		virtual ~EditorWndBase();
	};

	class EditorFuncWnd: public EditorWndBase {
	private:
		Func<void()> m_Func;
	public:
		EditorFuncWnd(const char* name, const Func<void()>& func, ImGuiWindowFlags flags = ImGuiWindowFlags_None);
		void Update() override;
		void Display() override;
	};
}
	