#pragma once
#include "EditorUI.h"

namespace Editor {
	class FPSWindow :public EditorWindowBase {
	public:
		FPSWindow() : EditorWindowBase("FPS") {};
	private:
		void OnWindow() override;
	};

	class ObjectsWindow : public EditorWindowBase {
	public:
		ObjectsWindow() : EditorWindowBase("Objects") {}
	private:
		void OnWindow() override;
	};

	class DetailWindow : public EditorWindowBase {
	public:
		DetailWindow() : EditorWindowBase("Details") {}
	private:
		void OnWindow() override;
	};

	class ViewportWindow : public EditorWindowBase {
	private:
		bool m_MouseDown{ false };
		URect m_Viewport{ 0,0,0,0 };
		float m_LastX = 0.0f;
		float m_LastY = 0.0f;
		bool  m_ViewportShow{ false };
	public:
		ViewportWindow() : EditorWindowBase("Viewport", ImGuiWindowFlags_NoBackground) {}
	private:
		void CameraControl();
		void Update() override;
		void OnWindow() override;
	};
}
	