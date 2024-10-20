#include "EditorUI/Public/EditorWindow.h"
#include "Functions/Public/AssetManager.h"
#ifdef _WIN32
#include <Windows.h>
#endif

namespace Editor {

	EditorWndBase::EditorWndBase(const char* name, ImGuiWindowFlags flags, EWndUpdateOrder order):
		m_Name(name), m_Flags(flags), m_Order(order), m_LastEnable(true), m_Enable(true), m_ToDelete(false), m_AutoDelete(false) {}

	void EditorWndBase::Delete() {
		m_ToDelete = true;
	}

	void EditorWndBase::AutoDelete() {
		m_AutoDelete = true;
	}

	void EditorWndBase::Close() {
		m_Enable = false;
	}

	void EditorWndBase::Tick() {
		// check open or close
		if(m_Enable && !m_LastEnable) {
			OnOpen();
		}
		else if(!m_Enable && m_LastEnable) {
			OnClose();
		}
		m_LastEnable = m_Enable;

		if (m_Enable) {
			if (ImGui::Begin(m_Name.c_str(), &m_Enable, m_Flags)) {
				// support right click to focus window
				constexpr auto flag = ImGuiHoveredFlags_ChildWindows;// hover checking include child window
				if (ImGui::IsWindowHovered(flag) && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
					ImGui::SetWindowFocus();
				}
				WndContent();
			}
			ImGui::End();
		}
		if(m_AutoDelete && !m_Enable) {
			m_ToDelete = true;
		}
	}

	EditorFuncWnd::EditorFuncWnd(const char* name, const Func<void()>& func, ImGuiWindowFlags flags) : EditorWndBase(name, flags, EWndUpdateOrder::Order1), m_Func(func) {
		AutoDelete();
	}
	void EditorFuncWnd::WndContent() {
		m_Func();
	}
}
