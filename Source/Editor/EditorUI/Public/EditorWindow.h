#pragma once
#include "EditorUI.h"

namespace Editor {
	class FPSWindow :public EWindowBase {
	public:
		FPSWindow() : EWindowBase("FPS") {};
	private:
		void OnWindow() override;
	};

	class ObjectsWindow : public EWindowBase {
	public:
		ObjectsWindow() : EWindowBase("Objects") {}
	private:
		void OnWindow() override;
	};

	class DetailWindow : public EWindowBase {
	public:
		DetailWindow() : EWindowBase("Details") {}
	private:
		void OnWindow() override;
	};

	class AssetsWindow : public EWindowBase {
	public:
		AssetsWindow() : EWindowBase("Assets") {}
	private:
		void OnWindow() override;
	};

	class ViewportWindow : public EWindowBase {
	private:
		bool m_MouseDown{ false };
		URect m_Viewport{ 0,0,0,0 };
		float m_LastX = 0.0f;
		float m_LastY = 0.0f;
		bool  m_ViewportShow{ false };
	public:
		ViewportWindow() : EWindowBase("Viewport", ImGuiWindowFlags_NoBackground) {}
	private:
		void CameraControl();
		void Update() override;
		void OnWindow() override;
	};
}
	