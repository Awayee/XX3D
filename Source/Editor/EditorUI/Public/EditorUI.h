#pragma once
#include "Render/Public/ImGuiImpl.h"
#include "Render/Public/InputEnum.h"
#include "Objects/Public/Engine.h"

namespace Editor {
	class EWindowBase {
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
		EWindowBase(const char* name, ImGuiWindowFlags flags=ImGuiWindowFlags_None) : m_Name(name), m_Flags(flags){}
		virtual ~EWindowBase() = default;
		void Run(); // inner func
		void SetShow(bool show) { m_Show = show; }
	};

	class UIMgr {
	private:
		TVector<TUniquePtr<EWindowBase>> m_Windows;
		ImGuiContext* m_Context{ nullptr };
		void InitWindows();
	public:
		UIMgr();
		~UIMgr();
		void Tick();
	};
}
