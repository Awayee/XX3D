#pragma once
#include "Functions/Public/AssetManager.h"
#include "EditorUI/Public/EditorWindow.h"
#include "AssetView.h"

namespace Editor {

	class WndAssetBrowser : public EditorWndBase {

	public:
		WndAssetBrowser();
		~WndAssetBrowser() override;
		void WndContent() override;
		void SetCurrentFolder(FolderNode* node);

	private:
		static constexpr NodeID MAX_FOLDER_NUM = 0xffff;
		static TArray<WndAssetBrowser*> s_Instances;
		FolderNode* m_CurrentFolder{nullptr};
		TArray<TUniquePtr<AssetViewBase>> m_Contents;
		uint32 m_SelectedIdx{ INVALID_INDEX };
		uint32 m_RenamingIdx{ INVALID_INDEX };
		TStaticArray<char, 64> m_RenameStr;

		static void OnFolderRebuildAllWindows(const FolderNode* node);
		void OnFolderRebuild(const FolderNode* node);
		void RefreshItems();
		void DisplayFolderTree(NodeID node);
		void DisplayContents();
		void EnterRenameByIdx(uint32 idx);
		void EnterRenameByNodeID(NodeID nodeID);
		void ExitRename();
		XString GenerateNewFilePath(const char* ext);
	};
	
}
