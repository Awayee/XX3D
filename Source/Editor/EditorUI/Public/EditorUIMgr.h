#pragma once
#include "Core/Public/TSingleton.h"
#include "Core/Public/TVector.h"
#include "Core/Public/SmartPointer.h"
#include "Core/Public/Func.h"
#include "EditorUI/Public/Widget.h"
#include "EditorUI/Public/EditorWindow.h"
#include "Core/Public/String.h"

namespace Editor {

	class EditorUIMgr: public TSingleton<EditorUIMgr>{
	private:
		friend TSingleton<EditorUIMgr>;
		struct MenuItem {
			String Name;
			Func<void()> Func;
			bool* Toggle;
		};
		struct MenuColumn {
			String Name;
			TVector<MenuItem> Items;
		};
		TVector<MenuColumn> m_MenuBar;
		TVector<TUniquePtr<EditorWindowBase>> m_Windows;
		EditorUIMgr();
		~EditorUIMgr();
	public:
		void Tick();
		void AddMenuBar(const char* barName);
		void AddMenu(const char* barName, const char* name, Func<void()>&& func, bool* pToggle);
		EditorWindowBase* AddWindow(const char* name, Func<void()>&& func, ImGuiWindowFlags flags=ImGuiWindowFlags_None);
		void AddWindow(TUniquePtr<EditorWindowBase>&& wnd);
		void DeleteWindow(EditorWindowBase*& pWnd);
		void RemoveWindow(WidgetID wndId);
	};
}
