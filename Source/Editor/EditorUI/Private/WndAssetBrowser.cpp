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
		AssetBuilderBase(XString&& wndName, Editor::NodeID folderNode): EditorWndBase(MoveTemp(wndName), WINDOW_FLAGS), m_FolderNode(folderNode){
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

	class InstanceDataBuilder: public AssetBuilderBase {
	public:
		InstanceDataBuilder(Editor::NodeID folderNode) : AssetBuilderBase("Instance Data Builder", folderNode){}
	private:
		Math::FVector2 m_BatchStartPos{ 0, 0 };
		Math::FVector2 m_BatchEndPos{ 128.0f, 128.0f };
		Math::IVector2 m_BatchDensity{ 128, 128 };
		float m_BatchUniformHeight{ 0.0f };
		int m_InstanceSize{ 0 };
		int m_InstanceEditIndex{ 0 };
		bool m_RandomXZ{ true };
		TArray<Math::FTransform> m_Instances;
		
		void WndContent() override {
			ImGui::Text("Build to %s", GetPathStr().c_str());
			ImGui::InputText("Dst File Name", m_DstFileName.Data(), m_DstFileName.ByteSize());
			if(ImGui::TreeNode("Random Instance")) {
				ImGui::DragFloatRange2("X Range", &m_BatchStartPos.X, &m_BatchEndPos.X, -9999.0f, 9999.0f);
				ImGui::DragFloatRange2("Y Range", &m_BatchStartPos.Y, &m_BatchEndPos.Y, -9999.0f, 9999.0f);
				ImGui::DragInt2("Density", m_BatchDensity.Data(), 1, 1, 9999);
				ImGui::DragFloat("Height", &m_BatchUniformHeight);
				ImGui::Checkbox("Random XZ", &m_RandomXZ);
				if(ImGui::Button("Generate")) {
					m_InstanceSize = m_BatchDensity.X * m_BatchDensity.Y;
					m_Instances.Resize((uint32)m_InstanceSize, Math::FTransform::IDENTITY);
					const Math::FVector2 delta{ (m_BatchEndPos.X - m_BatchStartPos.X) / (float)m_BatchDensity.X, (m_BatchEndPos.Y - m_BatchStartPos.Y) / (float)m_BatchDensity.Y };
					for (int dy = 0; dy < m_BatchDensity.Y; ++dy) {
						for (int dx = 0; dx < m_BatchDensity.X; ++dx) {
							Math::FVector2 pos = m_BatchStartPos + delta * Math::FVector2{ (float)dx, (float)dy };
							if(m_RandomXZ) {
								pos += Math::FVector2{Util::RandomF(0.0f, delta.X), Util::RandomF(0.0f, delta.Y)};
							}
							m_Instances[dy * m_BatchDensity.X + dx].Position = {pos.X, m_BatchUniformHeight, pos.Y};
						}
					}
				}
				ImGui::TreePop();
			}
			if(m_InstanceSize) {
				ImGui::SliderInt("Instance Edit Index", &m_InstanceEditIndex, 0, (int)m_Instances.Size()-1);
				if (m_InstanceEditIndex < m_InstanceSize) {
					Math::FTransform& transform = m_Instances[m_InstanceEditIndex];
					ImGui::DragFloat3("Position", transform.Position.Data(), 0.1f);
					Math::FVector3 euler = transform.Rotation.ToEuler();
					if (ImGui::DragFloat3("Rotation", euler.Data(), 0.1f)) {
						transform.Rotation = Math::FQuaternion::Euler(euler);
					}
					ImGui::DragFloat3("Scale", transform.Scale.Data(), 0.1f, 0.001f, 1000.0f);
				}
			}
			if(ImGui::Button("Create")) {
				File::FPath filePath = GetPath(); filePath.append(m_DstFileName.Data()).replace_extension(".instd");
				if(Asset::InstancedMeshAsset::SaveInstanceFile(filePath.string().c_str(), m_Instances)) {
					LOG_INFO("Instance file created: %s", m_DstFileName.Data());
					RefreshNode(MoveTemp(filePath));
					Close();
				}
				else {
					LOG_WARNING("Failed to create instance file: %s", m_DstFileName.Data());
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
			Browser()->RegisterFolderModifiedEvent(WndAssetBrowser::OnFolderRebuildAllWindows);
		}
		s_Instances.PushBack(this);
		SetCurrentFolder(Browser()->GetRootNode());
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
		for (uint32 i = 0; i < m_Contents.Size(); ++i) {
			auto& item = m_Contents[i];

			if (ImGui::Selectable(item->GetName().c_str(), m_SelectedItem == i, ImGuiSelectableFlags_AllowDoubleClick)) {
				m_SelectedItem = i;
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

		if (ImGui::BeginPopupContextWindow("ContextMenu", ImGuiPopupFlags_MouseButtonRight)) {
			const NodeID folderNode = m_CurrentFolder->GetID();
			Editor::EditorUIMgr* uiMgr = Editor::EditorUIMgr::Instance();
			if(ImGui::BeginMenu("New")) {
				if(ImGui::MenuItem("Instance Data")) {
					uiMgr->AddWindow(TUniquePtr(new InstanceDataBuilder(folderNode)));
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
			ImGui::EndPopup();
		}
		ImGui::EndChild();
	}
}
