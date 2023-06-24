#pragma once
#include "Widget.h"
#include "Core/Public/Func.h"

namespace Editor {

	class EditorWindowBase: public WidgetBase {
		friend class EditorUIMgr;
	protected:
		bool m_Enable{ true };
		bool m_ToDelete{ false };//mark this windows is to delete
		const char* m_Name = "Unknown";
		ImGuiWindowFlags m_Flags = ImGuiWindowFlags_None;
	protected:
		virtual void Update() = 0;
		virtual void Display() = 0;
	public:
		EditorWindowBase(const char* name, ImGuiWindowFlags flags = ImGuiWindowFlags_None);
		virtual ~EditorWindowBase();
		void Delete();
	};

	class EditorFuncWnd: public EditorWindowBase {
	private:
		Func<void()> m_Func;
	public:
		EditorFuncWnd(const char* name, const Func<void()>& func, ImGuiWindowFlags flags = ImGuiWindowFlags_None);
		void Update() override;
		void Display() override;
	};
}
	