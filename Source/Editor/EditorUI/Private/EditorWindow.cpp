#include "EditorUI/Public/EditorWindow.h"
#include "Functions/Public/AssetManager.h"
#ifdef _WIN32
#include <Windows.h>
#endif

namespace Editor {

	void EditorWndBase::Delete() {
		m_ToDelete = true;
	}

	void EditorWndBase::AutoDelete() {
		m_AutoDelete = true;
	}

	void EditorWndBase::Close() {
		m_Enable = false;
	}

	void EditorWndBase::Display() {
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

	void EditorFuncWnd::WndContent() {
		m_Func();
	}
}
