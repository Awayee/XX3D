#pragma once
#include "Core/Public/TArray.h"
#include "Core/Public/TUniquePtr.h"
#include "Core/Public/Func.h"
#include "EditorUI/Public/EditorWindow.h"
#include "Core/Public/String.h"
#include "RHI/Public/RHIImGui.h"

namespace Editor {

	class EditorUIMgr{
		SINGLETON_INSTANCE(EditorUIMgr);
	public:
		void Tick();
		void AddMenuBar(const char* barName);
		void AddMenu(const char* barName, const char* name, Func<void()>&& func, bool* pToggle);
		EditorWndBase* AddWindow(const char* name, Func<void()>&& func, ImGuiWindowFlags flags = ImGuiWindowFlags_None);
		void AddWindow(TUniquePtr<EditorWndBase>&& wndPtr);
		RHITexture* GetSceneTexture() { return m_SceneTexture; }
		void SetSceneTexture(RHITexture* texture) { m_SceneTexture = texture; }
	private:
		struct MenuItem {
			XString Name;
			Func<void()> Func;
			bool* Toggle;
		};
		struct MenuColumn {
			XString Name;
			TArray<MenuItem> Items;
		};
		TArray<MenuColumn> m_MenuBar;
		TStaticArray<TArray<TUniquePtr<EditorWndBase>>, EnumCast(EWndUpdateOrder::OrderMax)> m_Windows;
		RHITexture* m_SceneTexture{ nullptr };

		EditorUIMgr() = default;
		~EditorUIMgr() = default;
	};
}
