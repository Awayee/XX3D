#include "EditorUI/Public/EditorWindow.h"
#include "Objects/Public/RenderScene.h"
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

	void EditorWndBase::Display() {
		if (m_Enable) {
			if (ImGui::Begin(m_Name.c_str(), &m_Enable, m_Flags)) {
				// support right click to focus window
				if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
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

	XString OpenFileDialog(const char* filter) {
#ifdef _WIN32
		OPENFILENAME ofn;
		char szFile[128];
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = NULL;
		ofn.lpstrFile = szFile;
		ofn.lpstrFile[0] = '\0';
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
		if (GetOpenFileName(&ofn) == TRUE){
			return ofn.lpstrFile;
		}
#endif
		return "";
	}
}
