#include "WndAssetBrowser.h"
#include "UIExtent.h"
#include "EditorUI/Public/EditorUIMgr.h"
#include "Functions/Public/AssetImporter.h"
#include "Util/Public/Random.h"

namespace {
	static constexpr uint32 INPUT_CHAR_SIZE = 128;
	static constexpr uint32 CUBE_MAP_FILE_NUM = 6;
	static constexpr int32  WINDOW_FLAGS = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking;
	static const TStaticArray<const char*, 5> s_ImageDownsizeNames{ "1", "1/2", "1/4", "1/8", "1/16" };

	class AssetBuilderBase : public Editor::EditorWndBase {
	public:
		AssetBuilderBase(const char* name, Editor::NodeID folderNode): EditorWndBase(name, WINDOW_FLAGS), m_FolderNode(folderNode){
			AutoDelete();
			strcpy(m_DstFileName.Data(), "NewAsset");
		}
	protected:
		Editor::NodeID m_FolderNode;
		TStaticArray<char, INPUT_CHAR_SIZE> m_DstFileName{};
		const XString& GetPathStr() const { return Editor::ProjectAssetMgr::Instance()->GetFolderNode(m_FolderNode)->GetPathStr(); }
		File::FPath GetPath() const { return Editor::ProjectAssetMgr::Instance()->GetFolderNode(m_FolderNode)->GetPath(); }
		void RefreshNode(File::FPath&& path) const { Editor::ProjectAssetMgr::Instance()->CreateFile(MoveTemp(path), m_FolderNode); }
	};

	class MeshImporter: public AssetBuilderBase {
	public:
		MeshImporter(Editor::NodeID folderNode): AssetBuilderBase("MeshImporter", folderNode) {}
	private:
		XString m_SrcFile;
		float m_UniformScale{ 1.0f };
		void WndContent() override {
			ImGui::Text("Import to %s", GetPathStr().c_str());
			// update the target filename
			if (ImGui::DraggableFileItemGlobal("Src File", m_SrcFile, "*.glb;*.gltf;*.fbx")) {
				XString nameStr = File::FPath{ m_SrcFile }.stem().string();
				StrCopy(m_DstFileName.Data(), nameStr.c_str());
			}
			ImGui::DragFloat("Uniform Scale", &m_UniformScale, 0.1f, 0.0001f, 9999.0f);
			ImGui::InputText("Dst File Name", m_DstFileName.Data(), m_DstFileName.ByteSize());
			if (ImGui::Button("Import")) {
				File::FPath filePath = GetPath(); filePath.append(m_DstFileName.Data()).replace_extension(".mesh");
				if(ImportMeshAsset(m_SrcFile, filePath.string(), m_UniformScale)) {
					LOG_INFO("Mesh imported: %s", m_SrcFile.c_str());
					RefreshNode(MoveTemp(filePath));
				}
				else {
					LOG_WARNING("Failed to import mesh: %s", m_SrcFile.c_str());
				}
			}
		}
	};

	class TextureImporter: public AssetBuilderBase {
	public:
		TextureImporter(Editor::NodeID folderNode): AssetBuilderBase("TextureImporter", folderNode){}
	private:
		XString m_SrcFile;
		int m_DownsizeIdx{ 0 };
		int m_CompressMode{(int)Asset::ETextureCompressMode::Zlib};
		void WndContent() override {
			ImGui::Text("Import to %s", GetPathStr().c_str());
			// update the target filename
			if (ImGui::DraggableFileItemGlobal("Src File", m_SrcFile, "*.jpg;*.png*.tga")) {
				XString nameStr = File::FPath{ m_SrcFile }.stem().string();
				StrCopy(m_DstFileName.Data(), nameStr.c_str());
			}
			ImGui::InputText("Dst File Name", m_DstFileName.Data(), m_DstFileName.ByteSize());
			// size scale
			ImGui::Combo("Size Scale", &m_DownsizeIdx, s_ImageDownsizeNames.Data(), s_ImageDownsizeNames.Size());
			// compress mode
			ImGui::Combo("Compress Mode", &m_CompressMode, "None\0Lz4\0zlib");

			if (ImGui::Button("Import")) {
				File::FPath filePath = GetPath(); filePath.append(m_DstFileName.Data()).replace_extension(".texture");
				if (ImportTexture2DAsset(m_SrcFile, filePath.string(), 1 << m_DownsizeIdx, (Asset::ETextureCompressMode)m_CompressMode)) {
					LOG_INFO("Texture imported: %s", m_SrcFile.c_str());
					RefreshNode(MoveTemp(filePath));
				}
				else {
					LOG_WARNING("Failed to import texture: %s", m_SrcFile.c_str());
				}
			}
		}
	};

	class CubeMapImporter: public AssetBuilderBase {
	public:
		CubeMapImporter(Editor::NodeID folderNode): AssetBuilderBase("CubeMapImporter", folderNode) {}
	private:
		TStaticArray<XString, CUBE_MAP_FILE_NUM> m_SrcFiles;
		int m_DownsizeIdx{ 0 };
		int m_CompressMode{ (int)Asset::ETextureCompressMode::Zlib };
		void WndContent() override {
			static const char* srcNames[CUBE_MAP_FILE_NUM] = {"right ", "left  ", "top   ", "bottom", "front ", "back  " };
			ImGui::Text("Import to %s", GetPathStr().c_str());
			for (uint32 i = 0; i < CUBE_MAP_FILE_NUM; ++i) {
				if (ImGui::DraggableFileItemGlobal(srcNames[i], m_SrcFiles[i], "*.jpg;*.png") && i == 0) {
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
				File::FPath filePath = GetPath(); filePath.append(m_DstFileName.Data()).replace_extension(".texture");
				if(ImportTextureCubeAsset(m_SrcFiles, filePath.string(), 1 << m_DownsizeIdx, (Asset::ETextureCompressMode)m_CompressMode)) {
					LOG_INFO("Cubemap imported: %s", m_DstFileName.Data());
					RefreshNode(MoveTemp(filePath));
				}
				else {
					LOG_WARNING("Failed to import cubemap: %s", m_DstFileName.Data());
				}
			}
		}
	};

	bool CreateDefaultInstanceData(const XString& pathStr) {
		Asset::InstanceDataAsset asset;
		constexpr uint32 instanceSize = 32;
		asset.Instances.Reserve(instanceSize);
		for(uint32 i=0; i<instanceSize; ++i) {
			for(uint32 j=0; j<instanceSize; ++j) {
				auto& instance = asset.Instances.EmplaceBack();
				instance.Rotation = Math::FQuaternion::IDENTITY;
				instance.Scale = Math::FVector3::ONE;
				instance.Position = { (float)i, 0.0f, (float)j };
			}
		}
		return Asset::AssetLoader::SaveProjectAsset(&asset, pathStr.c_str());
	}
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
			Browser()->RegisterFolderModifiedEvent(WndAssetBrowser::OnFolderRebuildAllWindows);
		}
		s_Instances.PushBack(this);
		SetCurrentFolder(Browser()->GetRootNode());
	}

	WndAssetBrowser::~WndAssetBrowser() {
		s_Instances.SwapRemove(this);
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
		if(m_CurrentFolder && (m_CurrentFolder == node || node->Contains(m_CurrentFolder))) {
			RefreshItems();
		}
	}

	void WndAssetBrowser::RefreshItems() {
		if(!m_CurrentFolder) {
			return;
		}
		m_Contents.Reset();
		for(NodeID id : m_CurrentFolder->GetChildFolders()) {
			FolderNode* node = Browser()->GetFolderNode(id);
			if(node) {
				m_Contents.PushBack(TUniquePtr<FolderAssetView>(new FolderAssetView(node)));
			}
		}
		for(NodeID id : m_CurrentFolder->GetChildFiles()) {
			FileNode* node = Browser()->GetFileNode(id);
			if(node) {
				m_Contents.PushBack(CreateAssetView(node));
			}
		}
	}

	void WndAssetBrowser::DisplayFolderTree(NodeID node) {
		FolderNode* folderNode = Browser()->GetFolderNode(node);
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
		if (m_CurrentFolder->ParentFolder() != INVALID_NODE) {
			if (ImGui::ArrowButton("Back", ImGuiDir_Left)) {
				m_CurrentFolder = Browser()->GetFolderNode(m_CurrentFolder->ParentFolder());
				RefreshItems();
				return;
			}
		}
		else {
			ImGui::ArrowButton("Back", ImGuiDir_Left);
		}

		ImGui::BeginChild("Assets");
		uint32 mouseOnIdx = INVALID_INDEX;
		for (uint32 i = 0; i < m_Contents.Size(); ++i) {
			auto& item = m_Contents[i];

			if(m_RenamingIdx == i) {
				ImGui::SetKeyboardFocusHere();
				if (ImGui::InputText("##", m_RenameStr.Data(), m_RenameStr.Size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
					item->Rename(m_RenameStr.Data());
					ExitRename();
				}
			}
			else {
				if (ImGui::Selectable(item->GetName().c_str(), m_SelectedIdx == i, ImGuiSelectableFlags_AllowDoubleClick)) {
					m_SelectedIdx = i;
					if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
						if (item->IsFolder()) {
							SetCurrentFolder(Browser()->GetFolderNode(item->m_ID));
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

			if (ImGui::IsItemHovered()) {
				mouseOnIdx = i;
			}
		}

		// right click to select
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
			m_SelectedIdx = mouseOnIdx;
		}

		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			if (m_RenamingIdx != mouseOnIdx) {
				ExitRename();
			}
		}
		if (ImGui::IsKeyReleased(ImGuiKey_Escape)) {
			ExitRename();
		}

		if (ImGui::BeginPopupContextWindow("ContextMenu_Asset", ImGuiPopupFlags_MouseButtonRight)) {
			ExitRename();
			const NodeID folderNode = m_CurrentFolder->GetID();
			Editor::EditorUIMgr* uiMgr = Editor::EditorUIMgr::Instance();
			if(ImGui::BeginMenu("New")) {
				if(ImGui::MenuItem("Instance Data")) {
					XString filePath = GenerateNewFilePath(".instd");
					if(CreateDefaultInstanceData(filePath)) {
						const NodeID newNodeID = Editor::ProjectAssetMgr::Instance()->CreateFile(MoveTemp(filePath), m_CurrentFolder->GetID());
						if(INVALID_NODE != newNodeID) {
							EnterRenameByNodeID(newNodeID);
						}
					}
				}
				if(ImGui::MenuItem("Material")) {
					
				}
				ImGui::EndMenu();
			}
			if(ImGui::BeginMenu("Import")) {
				if(ImGui::MenuItem("Mesh")) {
					uiMgr->AddWindow(TUniquePtr(new MeshImporter(folderNode)));
				}
				if (ImGui::MenuItem("Texture")) {
					uiMgr->AddWindow(TUniquePtr(new TextureImporter(folderNode)));
				}
				if(ImGui::MenuItem("CubeMap")) {
					uiMgr->AddWindow(TUniquePtr(new CubeMapImporter(folderNode)));
				}
				ImGui::EndMenu();
			}
			if(ImGui::MenuItem("Show in Explore")) {
				File::FPath fullPath = m_CurrentFolder->GetFullPath();
				ImGui::OpenFileExplorer(fullPath.string().c_str());
			}
			if(ImGui::MenuItem("Rename")) {
				EnterRenameByIdx(m_SelectedIdx);
			}
			ImGui::EndPopup();
		}
		ImGui::EndChild();
	}

	void WndAssetBrowser::EnterRenameByIdx(uint32 idx) {
		if(m_RenamingIdx != idx) {
			AssetViewBase* view = m_Contents[idx].Get();
			if(view->IsFolder()) {
				FolderNode* folderNode = Browser()->GetFolderNode(view->m_ID);
				strcpy(m_RenameStr.Data(), folderNode->GetName().c_str());
			}
			else {
				FileNode* fileNode = Browser()->GetFileNode(view->m_ID);
				const XString nameWithoutExt = fileNode->GetNameWithoutExt();
				strcpy(m_RenameStr.Data(), nameWithoutExt.c_str());
			}
			m_RenamingIdx = idx;
		}
	}

	void WndAssetBrowser::EnterRenameByNodeID(NodeID nodeID) {
		for(uint32 i=0; i<m_Contents.Size(); ++i) {
			if(m_Contents[i]->m_ID == nodeID) {
				EnterRenameByIdx(i);
				break;
			}
		}
	}

	void WndAssetBrowser::ExitRename() {
		m_RenamingIdx = INVALID_INDEX;
	}

	XString WndAssetBrowser::GenerateNewFilePath(const char* ext) {
		uint32 identityNum = 0;
		File::FPath fileFullPath = m_CurrentFolder->GetFullPath(); fileFullPath.append("NewAsset");
		fileFullPath.replace_extension(ext);
		while(File::Exist(fileFullPath)) {
			XString fileName = StringFormat("NewAsset%u", identityNum++);
			fileFullPath = m_CurrentFolder->GetFullPath();
			fileFullPath.append(fileName);
			fileFullPath.replace_extension(ext);
		}
		return fileFullPath.string();
	}
}
