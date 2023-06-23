#pragma once
#include "EditorUI/Public/Widget.h"
#include "Functions/Public/AssetManager.h"
#include "EditorUI/Public/EditorWindow.h"
#include "AssetView.h"

namespace Editor {

	class AssetsBrowser : public EditorWndBase {
		const NodeID MAX_FOLDER_NUM = 0xffff;

	private:
		static TVector<AssetsBrowser*> s_Instances;

		FolderNode* m_CurrentFolder{nullptr};
		TVector<TUniquePtr<AssetViewBase>> m_Contents;

		uint32 m_SelectedItem{ INVALLID_NODE };
	private:
		static void OnFolderRebuildAllWindows(const FolderNode* node);
		void OnFolderRebuild(const FolderNode* node);
		void RefreshItems();

	public:
		AssetsBrowser();
		~AssetsBrowser() override;
		void Update() override;
		void Display() override;
	};
	
}
