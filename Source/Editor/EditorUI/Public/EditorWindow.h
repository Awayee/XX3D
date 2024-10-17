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
		explicit EditorWndBase(const char* name, ImGuiWindowFlags flags=ImGuiWindowFlags_None, EWndUpdateOrder order=EWndUpdateOrder::Order0);
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
		EWndUpdateOrder m_Order;
		bool m_Enable;
		bool m_LastEnable : 1;
		bool m_ToDelete : 1;//mark this widget is to delete
		bool m_AutoDelete : 1;
		void Tick();
		virtual void WndContent() {/*Do nothing*/}
		virtual void OnOpen() {/*Do nothing*/}
		virtual void OnClose() {/*Do nothing*/}
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
	