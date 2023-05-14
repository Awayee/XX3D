#pragma once
#include "Window/Public/ImGuiImpl.h"
#include "Window/Public/InputEnum.h"
#include "Objects/Public/Engine.h"

namespace Editor {
	class EditorWindowBase {
		friend class UIMgr;
	protected:
		bool m_Show{ true };
		const char* m_Name = "Invalid";
		ImGuiWindowFlags m_Flags;
		virtual void Update() {}
		virtual void PreWindow() {}
		virtual void OnWindow() = 0;
		virtual void PostWindow() {}
	public:
		EditorWindowBase(const char* name, ImGuiWindowFlags flags=ImGuiWindowFlags_None) : m_Name(name), m_Flags(flags){}
		virtual ~EditorWindowBase() = default;
		void Run(); // inner func
		void SetShow(bool show) { m_Show = show; }
	};

	class UIMgr {
	private:
		TVector<TUniquePtr<EditorWindowBase>> m_Windows;
		ImGuiContext* m_Context{ nullptr };
		void InitWindows();
	public:
		UIMgr();
		~UIMgr();
		void Tick();
	};
}
