#pragma once
#include "EditorUI/Public/EditorWindow.h"

namespace Editor {

	class WndDebugView : public Editor::EditorWndBase {
	public:
		WndDebugView();
		~WndDebugView() override;
		void WndContent() override;
	private:
		enum DebugViewMode : int{
			DV_DIRECTIONAL_SHADOW,
		};
		int m_ViewMode;
		int m_ShadowMapArrayIdx;
		TArray<ImTextureID> m_ShadowMapImGuiIDs;
		RHITexture* m_CachedShadowMap; // cache
		void ShowShadowMap();
		void ReleaseShadowMapIDs();
	};
}
