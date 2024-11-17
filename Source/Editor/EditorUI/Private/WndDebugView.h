#pragma once
#include "EditorUI/Public/EditorWindow.h"

namespace Editor {

	class DebugViewBase {
	public:
		DebugViewBase();
		virtual ~DebugViewBase();
		virtual RHITexture* GetTexture() = 0;
		virtual ETextureViewFlags GetTextureViewFlags() = 0;
		virtual RHISampler* GetSampler() = 0;
		virtual void Display();
		void Release();
	protected:
		TArray<ImTextureID> m_ImGuiIDs;
		RHITexture* m_CacheTexture;
		int m_ArraySize;
		int m_MipSize;
		int m_ArrayIndex;
		int m_MipIndex;
	};

	class WndDebugView : public Editor::EditorWndBase {
	public:
		WndDebugView();
		~WndDebugView() override;
		void WndContent() override;
	private:
		enum DebugViewMode : int {
			DV_DIRECTIONAL_SHADOW,
			DV_HZB,
			DV_COUNT,
		};
		int m_ViewMode;
		TStaticArray<TUniquePtr<DebugViewBase>, DebugViewMode::DV_COUNT> m_DebugViews;
	};
}
