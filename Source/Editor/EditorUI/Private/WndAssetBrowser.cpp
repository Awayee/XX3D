#include "WndAssetBrowser.h"
#include "EditorUI/Public/EditorUIMgr.h"
#include "Functions/Public/AssetImporter.h"

namespace {
	static constexpr uint32 INPUT_CHAR_SIZE = 128;
	static constexpr uint32 CUBE_MAP_FILE_NUM = 6;
	static constexpr int32  WINDOW_FLAGS = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking;


	static const TStaticArray<const char*, 5> s_ImageDownsizeNames{ "1", "1/2", "1/4", "1/8", "1/16" };

	class AssetImporterBase : public Editor::EditorWndBase {
	public:
		AssetImporterBase(const XString& dstDir, XString&& fileFilter, XString&& wndName): EditorWndBase(MoveTemp(wndName), WINDOW_FLAGS), m_DstDir(dstDir), m_FileFilter(MoveTemp(fileFilter)) {
			AutoDelete();
		}
	protected:
		XString m_DstDir;
		XString m_FileFilter;
		TStaticArray<char, INPUT_CHAR_SIZE> m_DstFileName{};

		bool ReceiveDragItem(XString& file) {
			ImGui::Text(file.empty() ? "None" : file.c_str());
			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("File")) {
					const Editor::FileNode* fileNode = reinterpret_cast<const Editor::FileNode*>(payload->Data);
					if (m_FileFilter.find(fileNode->GetExt()) != XString::npos) {
						file = fileNode->GetFullPath().string();
					}
				}
				ImGui::EndDragDropTarget();
				return true;
			}
			return false;
		}
	};

	class MeshImporter: public AssetImporterBase {
	public:
		MeshImporter(const XString& dstDir): AssetImporterBase(dstDir, "*.glb;*.gltf;*.fbx", "MeshImporter") {}
	private:
		XString m_SrcFile;

		void WndContent() override {
			ImGui::Text("Import to %s", m_DstDir.c_str());
			bool isSrcDirty = false;
			if(ImGui::Button("SrcFile")) {
				isSrcDirty = true;
				m_SrcFile = Editor::OpenFileDialog(m_FileFilter.c_str());
			}
			ImGui::SameLine(); 
			if(ReceiveDragItem(m_SrcFile)) {
				isSrcDirty = true;
			}
			// update the target filename
			if (isSrcDirty) {
				XString nameStr = File::FPath{ m_SrcFile }.stem().string();
				StrCopy(m_DstFileName.Data(), nameStr.c_str());
			}
			ImGui::InputText("Dst File Name", m_DstFileName.Data(), m_DstFileName.ByteSize());

			if (ImGui::Button("Import")) {
				File::FPath dstFilePath{ m_DstDir };
				dstFilePath.append(m_DstFileName.Data()).replace_extension(".mesh");
				if(!ImportMeshAsset(m_SrcFile, dstFilePath.string())) {
					LOG_WARNING("Failed to import mesh: %s", m_SrcFile.c_str());
				}
				else {
					Delete();
				}
			}
		}
	};

	class TextureImporter: public AssetImporterBase {
	public:
		TextureImporter(const XString& dstDir): AssetImporterBase(dstDir, "*.jpg;*.png", "TextureImporter"){}
	private:
		XString m_SrcFile;
		int m_DownsizeIdx{ 0 };
		int m_CompressMode{(int)Asset::ETextureCompressMode::Zlib};

		void WndContent() override {
			ImGui::Text("Import to %s", m_DstDir.c_str());
			bool isSrcDirty = false;
			if (ImGui::Button("SrcFile")) {
				isSrcDirty = true;
				m_SrcFile = Editor::OpenFileDialog(m_FileFilter.c_str());
			}
			ImGui::SameLine();
			if (ReceiveDragItem(m_SrcFile)) {
				isSrcDirty = true;
			}
			// update the target filename
			if (isSrcDirty) {
				XString nameStr = File::FPath{ m_SrcFile }.stem().string();
				StrCopy(m_DstFileName.Data(), nameStr.c_str());
			}
			ImGui::InputText("Dst File Name", m_DstFileName.Data(), m_DstFileName.ByteSize());
			// size scale
			ImGui::Combo("Size Scale", &m_DownsizeIdx, s_ImageDownsizeNames.Data(), s_ImageDownsizeNames.Size());
			// compress mode
			ImGui::Combo("Compress Mode", &m_CompressMode, "None\0Lz4\0zlib");

			if (ImGui::Button("Import")) {
				File::FPath dstFilePath{ m_DstDir };
				dstFilePath.append(m_DstFileName.Data()).replace_extension(".texture");
				if (!ImportTexture2DAsset(m_SrcFile, dstFilePath.string(), 1 << m_DownsizeIdx, (Asset::ETextureCompressMode)m_CompressMode)) {
					LOG_WARNING("Failed to import texture: %s", m_SrcFile.c_str());
				}
				else {
					Delete();
				}
			}
		}
	};

	class CubeMapImporter: public AssetImporterBase {
	public:
		CubeMapImporter(const XString& dstDir): AssetImporterBase(dstDir, "*.jpg;*.png", "CubeMapImporter") {}
	private:
		TStaticArray<XString, CUBE_MAP_FILE_NUM> m_SrcFiles;
		int m_DownsizeIdx{ 0 };
		int m_CompressMode{ (int)Asset::ETextureCompressMode::Zlib };
		void WndContent() override {
			static const char* srcNames[CUBE_MAP_FILE_NUM] = {"right ", "left  ", "top   ", "bottom", "front ", "back  " };
			ImGui::Text("Import to %s", m_DstDir.c_str());
			for (uint32 i = 0; i < CUBE_MAP_FILE_NUM; ++i) {
				bool isSrcDirty = false;
				if (ImGui::Button(srcNames[i])) {
					isSrcDirty = true;
					m_SrcFiles[i] = Editor::OpenFileDialog("*.png;*.jpg");
				}
				ImGui::SameLine();
				if(ReceiveDragItem(m_SrcFiles[i])) {
					isSrcDirty = true;
				}
				if (i == 0 && isSrcDirty) {
					// update the target filename
					XString nameStr = File::FPath{ m_SrcFiles[i] }.stem().string();
					StrCopy(m_DstFileName.Data(), nameStr.c_str());
				}
			}
			ImGui::InputText("Dst File Name", m_DstFileName.Data(), m_DstFileName.ByteSize());
			// size scale
			ImGui::Combo("Size Scale", &m_DownsizeIdx, s_ImageDownsizeNames.Data(), s_ImageDownsizeNames.Size());
			// compress mode
			ImGui::Combo("Compress Mode", &m_CompressMode, "None\0Lz4\0zlib");

			if (ImGui::Button("Import")) {
				// check files valid
				for (uint32 i = 0; i < CUBE_MAP_FILE_NUM; ++i) {
					if (m_SrcFiles[i].empty() ||
						!(StrEndsWith(m_SrcFiles[i].c_str(), ".png") || StrEndsWith(m_SrcFiles[i].c_str(), ".jpg"))) {
						LOG_WARNING("[ImportCubeMap] file %u error!", i);
						return;
					}
				}
				File::FPath dstFilePath{ m_DstDir };
				dstFilePath.append(m_DstFileName.Data()).replace_extension(".texture");
				if(!ImportTextureCubeAsset(m_SrcFiles, dstFilePath.string(), 1 << m_DownsizeIdx, (Asset::ETextureCompressMode)m_CompressMode)) {
					LOG_WARNING("Failed to import cubemap: %s", m_SrcFiles[0].c_str());
				}
				else {
					Delete();
				}
			}
		}
	};
}

namespace Editor {

	inline AssetManager* Browser() { return ProjectAssetMgr::Instance(); }

	inline bool IsMouseOnTreeNodeArrow() {
		ImVec2 arrowMin = ImGui::GetItemRectMin();
		ImVec2 arrowMax = ImGui::GetItemRectMax();
		arrowMax.x = arrowMin.x + ImGui::GetTreeNodeToLabelSpacing();
		return ImGui::IsMouseHoveringRect(arrowMin, arrowMax);
	}

	TArray<WndAssetBrowser*> WndAssetBrowser::s_Instances;

	WndAssetBrowser::WndAssetBrowser() : EditorWndBase("Assets") {
		EditorUIMgr::Instance()->AddMenu("Window", m_Name.c_str(), {}, &m_Enable);
		if (s_Instances.IsEmpty()) {
			Browser()->RegisterFolderRebuildEvent(WndAssetBrowser::OnFolderRebuildAllWindows);
		}
		s_Instances.PushBack(this);
		SetCurrentFolder(Browser()->GetRoot());
	}

	WndAssetBrowser::~WndAssetBrowser() {
		s_Instances.SwapRemove(this);
	}

	void WndAssetBrowser::Update() {
	}

	void WndAssetBrowser::WndContent() {

		ImGui::Columns(2);

		// folders
		ImGui::SetColumnWidth(0, 256.0f);
		DisplayFolderTree(Browser()->RootID());

		// files
		ImGui::NextColumn();
		DisplayContents();
	}

	void WndAssetBrowser::SetCurrentFolder(FolderNode* node) {
		m_CurrentFolder = node;
		RefreshItems();
	}

	void WndAssetBrowser::OnFolderRebuildAllWindows(const FolderNode* node) {
		for(WndAssetBrowser* wnd: s_Instances) {
			wnd->OnFolderRebuild(node);
		}
	}

	void WndAssetBrowser::OnFolderRebuild(const FolderNode* node) {
		if(m_CurrentFolder && node->Contains(m_CurrentFolder->GetID())) {
			RefreshItems();
		}
	}

	void WndAssetBrowser::RefreshItems() {
		if(!m_CurrentFolder) {
			return;
		}
		m_Contents.Reset();
		for(NodeID id : m_CurrentFolder->GetChildFolders()) {
			FolderNode* node = Browser()->GetFolder(id);
			if(node) {
				m_Contents.PushBack(TUniquePtr<FolderAssetView>(new FolderAssetView(node)));
			}
		}
		for(NodeID id : m_CurrentFolder->GetChildFiles()) {
			FileNode* node = Browser()->GetFile(id);
			if(node) {
				m_Contents.PushBack(CreateAssetView(node));
			}

		}
	}

	void WndAssetBrowser::DisplayFolderTree(NodeID node) {
		FolderNode* folderNode = Browser()->GetFolder(node);
		if(!folderNode) {
			return;
		}
		auto& children = folderNode->GetChildFolders();
		ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow;
		if(children.IsEmpty()) {
			nodeFlags |= ImGuiTreeNodeFlags_Leaf;
		}

		if(folderNode == m_CurrentFolder) {
			nodeFlags |= ImGuiTreeNodeFlags_Selected;
		}

		bool expanded = ImGui::TreeNodeEx(folderNode->GetName().c_str(), nodeFlags);
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
			if (children.IsEmpty() || !IsMouseOnTreeNodeArrow()) {
				SetCurrentFolder(folderNode);
			}
		}

		if(expanded){
			for(NodeID child: children) {
				DisplayFolderTree(child);
			}
			ImGui::TreePop();
		}
	}

	void WndAssetBrowser::DisplayContents() {
		if (m_CurrentFolder->ParentFolder() != INVALLID_NODE) {
			if (ImGui::ArrowButton("Back", ImGuiDir_Left)) {
				m_CurrentFolder = Browser()->GetFolder(m_CurrentFolder->ParentFolder());
				RefreshItems();
				return;
			}
		}
		else {
			ImGui::ArrowButton("Back", ImGuiDir_Left);
		}

		ImGui::BeginChild("Assets");
		for (uint32 i = 0; i < m_Contents.Size(); ++i) {
			auto& item = m_Contents[i];

			if (ImGui::Selectable(item->GetName().c_str(), m_SelectedItem == i, ImGuiSelectableFlags_AllowDoubleClick)) {
				m_SelectedItem = i;
				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
					if (item->IsFolder()) {
						SetCurrentFolder(Browser()->GetFolder(item->m_ID));
					}
					else {
						item->Open();
					}
				}
			}

			if (ImGui::BeginDragDropSource()) {
				ImGui::Text(item->GetName().c_str());
				item->OnDrag();
				ImGui::EndDragDropSource();
			}
		}

		int32 flag = ImGuiPopupFlags_MouseButtonRight;
		if (ImGui::BeginPopupContextWindow("ContextMenu", flag)) {
			if(ImGui::BeginMenu("Import")) {
				const XString& dstDir = m_CurrentFolder->GetPathStr();
				if(ImGui::MenuItem("Mesh")) {
					Editor::EditorUIMgr::Instance()->AddWindow(TUniquePtr(new MeshImporter(dstDir)));
				}
				if (ImGui::MenuItem("Texture")) {
					Editor::EditorUIMgr::Instance()->AddWindow(TUniquePtr(new TextureImporter(dstDir)));
				}
				if(ImGui::MenuItem("CubeMap")) {
					Editor::EditorUIMgr::Instance()->AddWindow(TUniquePtr(new CubeMapImporter(dstDir)));
				}
				ImGui::EndMenu();
			}
			ImGui::EndPopup();
		}

		ImGui::EndChild();
	}
}
