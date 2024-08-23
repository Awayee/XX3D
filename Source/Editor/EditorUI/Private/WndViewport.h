#pragma once
#include "EditorUI/Public/EditorWindow.h"
#include "Math/Public/Math.h"

namespace Object {
	class Camera;
}

namespace Editor {
	class WndViewport : public EditorWndBase {
	public:
		WndViewport();
		~WndViewport() override;
	private:
		RHITexturePtr m_RenderTarget;
		ImTextureID m_RenderTargetID;
		USize2D m_ViewportSize{ 0, 0 };
		Rect m_Viewport{ 0,0,0,0 };
		float m_LastX{ 0.0f };
		float m_LastY{ 0.0f	};
		bool m_MouseDown{ false };
		bool m_ViewportShow{ false };

		void CameraControl(Object::Camera* camera);
		void Update() override;
		void WndContent() override;
		void ReleaseRenderTarget();
		void CreateRenderTarget();
	};
}
