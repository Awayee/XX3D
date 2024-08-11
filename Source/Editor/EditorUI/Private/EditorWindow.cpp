#include "EditorUI/Public/EditorWindow.h"
#include "Objects/Public/RenderScene.h"

namespace Editor {


	void EditorWndBase::Update() {
	}

	void EditorWndBase::Display() {
		if(m_Enable) {
			if (ImGui::Begin(m_Name, &m_Enable, m_Flags)) {
				// support right click to focus window
				if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
					ImGui::SetWindowFocus();
				}
				WndContent();
			}
			ImGui::End();
		}
	}

	EditorWndBase::EditorWndBase(const char* name, ImGuiWindowFlags flags): m_Name(name), m_Flags(flags) {
	}

	EditorWndBase::~EditorWndBase() {
	}

	void EditorWndBase::Delete() {
		m_ToDelete = true;
	}

	EditorFuncWnd::EditorFuncWnd(const char* name, const Func<void()>& func, ImGuiWindowFlags flags): EditorWndBase(name, flags) {
		m_Func = func;
	}

	void EditorFuncWnd::Update() {
		m_ToDelete = !m_Enable;
	}

	void EditorFuncWnd::WndContent() {
		m_Func();
	}

	EditorPopup::EditorPopup(const Func<void()>& func) {
		m_Func = func;
	}

	EditorPopup::~EditorPopup() {
	}

	void EditorPopup::Update() {
	}

	void EditorPopup::Display() {
		XString str = StringFormat("Popup_%u", m_ID);
		if(ImGui::BeginPopupContextItem(str.c_str())) {
			m_Func();
			ImGui::EndPopup();
		}
	}
}
