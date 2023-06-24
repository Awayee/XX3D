#include "EditorUI/Public/EditorWindow.h"
#include "Render/Public/RenderScene.h"

namespace Editor {


	EditorWindowBase::EditorWindowBase(const char* name, ImGuiWindowFlags flags): m_Name(name), m_Flags(flags) {
	}

	EditorWindowBase::~EditorWindowBase() {
	}

	void EditorWindowBase::Delete() {
		m_ToDelete = true;
	}

	EditorFuncWnd::EditorFuncWnd(const char* name, const Func<void()>& func, ImGuiWindowFlags flags): EditorWindowBase(name, flags) {
		m_Func = std::move(func);
	}

	void EditorFuncWnd::Update() {
		m_ToDelete = !m_Enable;
	}

	void EditorFuncWnd::Display() {
		m_Func();
	}
}