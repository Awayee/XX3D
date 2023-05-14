#pragma once
#include "EditorUI/Public/EditorWindow.h"
#include "EditorAsset/Public/AssetBrowser.h"

namespace Editor {
	class AssetsWindow : public EditorWindowBase {
		enum class EDisplayType {
			FOLDER,
			MESH,
			TEXTURE,
			SCENE,
			UNKNOWN
		};
		const NodeID MAX_FOLDER_NUM = 0xffff;
		struct NodeItem {
			NodeID ID;
			EDisplayType Type;
			const char* Name;
			NodeItem(const FileNode* file);
			NodeItem(const FolderNode* folder);
		};

	private:
		static TVector<AssetsWindow*> s_Instances;

		FolderNode* m_CurrentFolder{nullptr};
		TVector<NodeItem> m_Contents;

		uint32 m_SelectedItem{ INVALLID_NODE };
	private:
		static void OnFolderRebuildAllWindows(const FolderNode* node);
		void OnFolderRebuild(const FolderNode* node);
		void RefreshItems();

	public:
		AssetsWindow();
		~AssetsWindow();
		void OnWindow() override;
	};
	
}
