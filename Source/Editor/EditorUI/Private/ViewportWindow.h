#pragma once
#include "EditorUI/Public/EditorWindow.h"

namespace Editor {
	class ViewportWindow : public EditorWndBase {
	private:
		bool m_MouseDown{ false };
		URect m_Viewport{ 0,0,0,0 };
		float m_LastX = 0.0f;
		float m_LastY = 0.0f;
		bool  m_ViewportShow{ false };
	public:
		ViewportWindow() : EditorWndBase("Viewport", ImGuiWindowFlags_NoBackground) {}
	private:
		void CameraControl();
		void Update() override;
		void Display() override;
	};
}
