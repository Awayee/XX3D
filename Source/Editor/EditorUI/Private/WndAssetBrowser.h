#pragma once
#include "Functions/Public/AssetManager.h"
#include "EditorUI/Public/EditorWindow.h"
#include "AssetView.h"

namespace Editor {

	class WndAssetBrowser : public EditorWndBase {

	public:
		WndAssetBrowser();
		~WndAssetBrowser() override;
		void Update() override;
		void WndContent() override;
		void SetCurrentFolder(FolderNode* node);

	private:
		static constexpr NodeID MAX_FOLDER_NUM = 0xffff;
		static TArray<WndAssetBrowser*> s_Instances;

		FolderNode* m_CurrentFolder{nullptr};
		TArray<TUniquePtr<AssetViewBase>> m_Contents;
		TArray<TUniquePtr<FolderAssetView>> m_Folders;

		uint32 m_SelectedItem{ INVALID_NODE };
		ImVec2 m_ContextMenuPos;

		static void OnFolderRebuildAllWindows(const FolderNode* node);
		void OnFolderRebuild(const FolderNode* node);
		void RefreshItems();
		void DisplayFolderTree(NodeID node);
		void DisplayContents();
	};
	
}
