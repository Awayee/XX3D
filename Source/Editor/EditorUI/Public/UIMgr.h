#pragma once
#include "Core/Public/TSingleton.h"
#include "Core/Public/TVector.h"
#include "Core/Public/SmartPointer.h"
#include "Core/Public/Func.h"
#include "EditorUI/Public/Widget.h"
#include "EditorUI/Public/EditorWindow.h"

namespace Editor {

	class UIMgr: public TSingleton<UIMgr>{
	private:
		friend TSingleton<UIMgr>;
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
		TVector<TUniquePtr<EditorWndBase>> m_Windows;
		UIMgr();
		~UIMgr();
	public:
		void Tick();
		void AddMenu(const char* barName, const char* name, Func<void()>&& func, bool* pToggle);
		void AddWindow(const char* name, Func<void()>&& func, ImGuiWindowFlags flags=ImGuiWindowFlags_None);
		void AddWindow(TUniquePtr<EditorWndBase>&& wnd);
	};
}
