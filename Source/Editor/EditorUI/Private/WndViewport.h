#pragma once
#include "EditorUI/Public/EditorWindow.h"
#include "Math/Public/Math.h"

namespace Object {
	class RenderCamera;
}

namespace Editor {
	class WndViewport : public EditorWndBase {
	public:
		WndViewport();
		~WndViewport() override;
	private:
		RHITexturePtr m_RenderTarget;
		ImTextureID m_RenderTargetID{ nullptr };
		USize2D m_ViewportSize{ 0, 0 };
		float m_LastX{ 0.0f };
		float m_LastY{ 0.0f	};
		bool m_MouseDown{ false };
		bool m_ViewportShow{ false };

		void CameraControl(Object::RenderCamera* camera);
		void WndContent() override;
		void SetupRenderTarget();
	};
}
