#pragma once
#include "EditorUI/Public/Widget.h"
#include "Functions/Public/AssetManager.h"
#include "EditorUI/Public/EditorWindow.h"
#include "AssetView.h"

namespace Editor {

	class WndAssetBrowser : public EditorWndBase {
		const NodeID MAX_FOLDER_NUM = 0xffff;

	private:
		static TArray<WndAssetBrowser*> s_Instances;

		FolderNode* m_CurrentFolder{nullptr};
		TArray<TUniquePtr<AssetViewBase>> m_Contents;
		TArray<TUniquePtr<FolderAssetView>> m_Folders;

		uint32 m_SelectedItem{ INVALLID_NODE };
		uint32 m_SelectedFolder{ INVALLID_NODE };
	private:
		static void OnFolderRebuildAllWindows(const FolderNode* node);
		void OnFolderRebuild(const FolderNode* node);
		void RefreshItems();
		void DisplayFolderTree(NodeID node);
		void DisplayContents();

	public:
		WndAssetBrowser();
		~WndAssetBrowser() override;
		void Update() override;
		void WndContent() override;
		void SetCurrentFolder(FolderNode* node);
	};
	
}
