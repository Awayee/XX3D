#pragma once
#include "Core/Public/Func.h"
#include "RHI/Public/RHIImGui.h"

namespace Editor {
	class EditorUIMgr;

	enum class EWndUpdateOrder : uint8 {
		Order0 = 0,
		Order1,
		Order2,
		Order3,
		OrderMax
	};
	// tab window
	class EditorWndBase {
	public:
		EditorWndBase(XString&& name, ImGuiWindowFlags flags=ImGuiWindowFlags_None, EWndUpdateOrder order=EWndUpdateOrder::Order0): m_Name(MoveTemp(name)), m_Flags(flags){}
		virtual ~EditorWndBase() = default;
		void Delete();
		void AutoDelete();
		void Close();
	protected:
		enum : uint32 {
			NAME_SIZE = 64,
			INVALID_INDEX = UINT32_MAX,
		};
		friend EditorUIMgr;
		XString m_Name;
		int32 m_Flags;
		bool m_ToDelete{ false };//mark this widget is to delete
		bool m_Enable{ true };
		bool m_AutoDelete{ false };
		EWndUpdateOrder m_Order{ EWndUpdateOrder::Order0 };
		void Display();
		virtual void Update() {}
		virtual void WndContent() = 0;
	};

	class EditorFuncWnd: public EditorWndBase {
	public:
		EditorFuncWnd(const char* name, const Func<void()>& func, ImGuiWindowFlags flags = ImGuiWindowFlags_None);
	private:
		friend EditorUIMgr;
		static constexpr EWndUpdateOrder FUNC_WND_ORDER{EWndUpdateOrder::Order1};
		Func<void()> m_Func;
		void WndContent() override;
	};
}
	