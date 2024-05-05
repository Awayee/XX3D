#pragma once
#include "Widget.h"
#include "Core/Public/Func.h"
#include "RHI/Public/ImGuiRHI.h"

namespace Editor {
	// tab window
	class EditorWndBase: public WidgetBase {
		friend class EditorUIMgr;
	protected:
		bool m_Enable{ true };
		const char* m_Name = "Unknown";
		ImGuiWindowFlags m_Flags = ImGuiWindowFlags_None;
	protected:
		virtual void Update() override;
		void Display() override;
		virtual void WndContent() = 0;
	public:
		EditorWndBase(const char* name, ImGuiWindowFlags flags = ImGuiWindowFlags_None);
		virtual ~EditorWndBase();
		void Delete();
	};

	class EditorFuncWnd: public EditorWndBase {
	private:
		Func<void()> m_Func;
	public:
		EditorFuncWnd(const char* name, const Func<void()>& func, ImGuiWindowFlags flags = ImGuiWindowFlags_None);
		void Update() override;
		void WndContent() override;
	};

	// popup window

	class EditorPopup: public WidgetBase {
	private:
		Func<void()> m_Func;
	public:
		EditorPopup(const Func<void()>& func);
		~EditorPopup() override;
		void Update() override;
		void Display() override;
	};
}
	