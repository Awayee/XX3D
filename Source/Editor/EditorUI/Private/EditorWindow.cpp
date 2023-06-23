#include "EditorUI/Public/EditorWindow.h"
#include "Render/Public/RenderScene.h"

namespace Editor {


	EditorWndBase::EditorWndBase(const char* name, ImGuiWindowFlags flags): m_Name(name), m_Flags(flags) {
	}

	EditorWndBase::~EditorWndBase() {
	}

	EditorFuncWnd::EditorFuncWnd(const char* name, const Func<void()>& func, ImGuiWindowFlags flags): EditorWndBase(name, flags) {
		m_Func = std::move(func);
	}

	void EditorFuncWnd::Update() {
	}

	void EditorFuncWnd::Display() {
		m_Func();
	}
}