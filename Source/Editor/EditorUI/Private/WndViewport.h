#pragma once
#include "EditorUI/Public/EditorWindow.h"

namespace Editor {
	class WndViewport : public EditorWndBase {
	private:
		bool m_MouseDown{ false };
		Rect m_Viewport{ 0,0,0,0 };
		float m_LastX = 0.0f;
		float m_LastY = 0.0f;
		bool  m_ViewportShow{ false };
	public:
		WndViewport();
	private:
		void CameraControl();
		void Update() override;
		void WndContent() override;
	};
}
