#pragma once
#include "Core/Public/TSingleton.h"
#include "Core/Public/TVector.h"
#include "Core/Public/TPtr.h"
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
		TVector<TUniquePtr<WidgetBase>> m_Widgets;
		EditorUIMgr();
		~EditorUIMgr();
	public:
		void Tick();
		void AddMenuBar(const char* barName);
		void AddMenu(const char* barName, const char* name, Func<void()>&& func, bool* pToggle);
		EditorWndBase* AddWindow(const char* name, Func<void()>&& func, ImGuiWindowFlags flags=ImGuiWindowFlags_None);
		void AddWindow(TUniquePtr<EditorWndBase>&& wnd);
		void DeleteWindow(EditorWndBase*& pWnd);
		EditorPopup* AddPopUp(Func<void()>&& func);
	};
}
