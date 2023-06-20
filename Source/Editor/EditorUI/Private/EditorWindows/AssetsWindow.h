#pragma once
#include "EditorUI/Public/EditorWindow.h"
#include "EditorAsset/Public/AssetManager.h"
#include "AssetView.h"

namespace Editor {

	class AssetsWindow : public EditorWindowBase {
		const NodeID MAX_FOLDER_NUM = 0xffff;

	private:
		static TVector<AssetsWindow*> s_Instances;

		FolderNode* m_CurrentFolder{nullptr};
		TVector<TUniquePtr<AssetViewBase>> m_Contents;

		uint32 m_SelectedItem{ INVALLID_NODE };
	private:
		static void OnFolderRebuildAllWindows(const FolderNode* node);
		void OnFolderRebuild(const FolderNode* node);
		void RefreshItems();

	public:
		AssetsWindow();
		~AssetsWindow() override;
		void OnWindow() override;
	};
	
}
