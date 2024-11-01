#include "AssetView.h"
#include "UIExtent.h"
#include "Functions/Public/EditorLevel.h"
#include "EditorUI/Public/EditorUIMgr.h"
#include "Render/Public/DefaultResource.h"
#include "Objects/Public/RenderResource.h"
#include "Util/Public/Random.h"
#include "Window/Public/EngineWindow.h"
#include "Asset/Public/MaterialAsset.h"
#include "Objects/Public/Material.h"
#include <imnodes.h>

namespace {
	class TextureViewer : public Editor::EditorWndBase{
	public:
		TextureViewer(const XString& pathStr): EditorWndBase("TextureViewer", ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_HorizontalScrollbar), m_PathStr(pathStr) {
			AutoDelete();
			if(!Asset::AssetLoader::LoadProjectAsset(&m_Asset, m_PathStr.c_str())) {
				return;
			}
			m_Texture = Object::StaticResourceMgr::CreateTextureFromAsset(m_Asset);
			// create imgui texture id
			auto& desc = m_Texture->GetDesc();
			const uint32 arraySize = desc.Get2DArraySize();
			m_TextureViewIDs.Resize(arraySize);
			auto* sampler = Render::DefaultResources::Instance()->GetDefaultSampler(ESamplerFilter::Bilinear, ESamplerAddressMode::Clamp);
			for(uint32 i=0; i<arraySize; ++i) {
				RHITextureSubRes subRes{ (uint16)i, 1, 0, 1, ETextureDimension::Tex2D, ETextureViewFlags::Color };
				m_TextureViewIDs[i] = RHIImGui::Instance()->RegisterImGuiTexture(m_Texture.Get(), subRes, sampler);
			}
			m_ArrayIndex = 0;
			// zoom to fit
			const float w = (float)desc.Width, h = (float)desc.Height;
			constexpr float uniformSize = 512.0f;
			m_Zoom = Math::Min(ZOOM_MAX, uniformSize / Math::Max(w, h));
		}
		~TextureViewer() {
			for(auto viewID: m_TextureViewIDs) {
				RHIImGui::Instance()->RemoveImGuiTexture(viewID);
			}
		}
		void WndContent() override {
			if(m_TextureViewIDs.IsEmpty()) {
				return;
			}
			const auto& desc = m_Texture->GetDesc();
			const float w = (float)desc.Width, h = (float)desc.Height;
			// zoom
			const float minSize = Math::Min(w, h);
			const float zoomMin = 2.0f / minSize;
			// info
			ImGui::Text("%u x %u", desc.Width, desc.Height);
			if(m_TextureViewIDs.Size() == 1) {
				m_ArrayIndex = 0;
			}
			else {
				ImGui::SliderInt("Array Index", &m_ArrayIndex, 0, (int)m_TextureViewIDs.Size() - 1);
			}
			// image
			ImGui::SliderFloat("Zoom", &m_Zoom, zoomMin, ZOOM_MAX);
			ImGui::BeginChild("Preview", { 0.0f, 0.0f }, ImGuiChildFlags_Border, ImGuiWindowFlags_HorizontalScrollbar);
			const ImVec2 textureSize{ m_Zoom * w, m_Zoom * h };
			ImGui::Image(m_TextureViewIDs[m_ArrayIndex], textureSize);
			if(ImGui::IsWindowHovered()) {
				// scroll zoom
				Engine::EngineWindow* engineWindow = Engine::EngineWindow::Instance();
				if(engineWindow->IsKeyDown(Engine::EKey::LeftCtrl)) {
					const float mouseWheel = ImGui::GetIO().MouseWheel;
					if(!Math::IsNearlyZero(mouseWheel)) {
						m_Zoom = Math::Clamp(m_Zoom + mouseWheel * 0.1f, zoomMin, ZOOM_MAX);
					}
				}
				else if(engineWindow->IsKeyDown(Engine::EKey::LeftShit)){}
				// check dragging
				else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
					m_Dragging = true;
				}
			}
			if(ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
				m_Dragging = false;
			}
			// drag image
			if (m_Dragging && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
				const ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
				const float newScrollX = Math::Clamp(ImGui::GetScrollX() - dragDelta.x, 0.0f, textureSize.x);
				const float newScrollY = Math::Clamp(ImGui::GetScrollY() - dragDelta.y, 0.0f, textureSize.y);
				ImGui::SetScrollX(newScrollX);
				ImGui::SetScrollY(newScrollY);
				ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
			}
			ImGui::EndChild();
		}
	private:
		XString m_PathStr;
		Asset::TextureAsset m_Asset;
		RHITexturePtr m_Texture;
		TArray<ImTextureID> m_TextureViewIDs;
		int m_ArrayIndex;
		float m_Zoom = 512.0f;
		bool m_Dragging = false;
		static constexpr float ZOOM_MAX{ 4.0f };
	};

	class MeshViewer: public Editor::EditorWndBase {
	public:
		MeshViewer(const XString& pathStr): EditorWndBase("MeshViewer", ImGuiWindowFlags_NoDocking), m_PathStr(pathStr) {
			AutoDelete();
			Asset::AssetLoader::LoadProjectAsset(&m_Asset, m_PathStr.c_str());
		}
	private:
		XString m_PathStr;
		Asset::MeshAsset m_Asset;
		void WndContent() override {
			for (auto& primitive : m_Asset.Primitives) {
				File::FPath primitivePath = primitive.BinaryFile;
				const XString primitiveName = File::FPath{ primitive.BinaryFile }.stem().string();
				ImGui::DraggableFileItemAssets(primitiveName.c_str(), primitive.MaterialFile, "*.matt;*.mati");
			}
			if (ImGui::Button("Save")) {
				Asset::AssetLoader::SaveProjectAsset(&m_Asset, m_PathStr.c_str());
			}
		}
	};

	class InstanceDataViewer : public Editor::EditorWndBase {
	public:
		InstanceDataViewer(const XString& pathStr) : EditorWndBase("Instance Data Viewer", ImGuiWindowFlags_NoDocking), m_PathStr(pathStr) {
			AutoDelete();
			Asset::AssetLoader::LoadProjectAsset(&m_Asset, pathStr.c_str());
		}
	private:
		XString m_PathStr;
		Asset::InstanceDataAsset m_Asset;
		Math::FVector2 m_BatchStartPos{ 0, 0 };
		Math::FVector2 m_BatchEndPos{ 128.0f, 128.0f };
		Math::IVector2 m_BatchDensity{ 128, 128 };
		float m_BatchUniformHeight{ 0.0f };
		int m_InstanceEditIndex{ 0 };
		bool m_RandomXZ{ true };

		void WndContent() override {
			auto& instances = m_Asset.Instances;
			if (ImGui::TreeNode("Random Instance")) {
				ImGui::DragFloatRange2("X Range", &m_BatchStartPos.X, &m_BatchEndPos.X, -9999.0f, 9999.0f);
				ImGui::DragFloatRange2("Y Range", &m_BatchStartPos.Y, &m_BatchEndPos.Y, -9999.0f, 9999.0f);
				ImGui::DragInt2("Density", m_BatchDensity.Data(), 1, 1, 9999);
				ImGui::DragFloat("Height", &m_BatchUniformHeight);
				ImGui::Checkbox("Random XZ", &m_RandomXZ);
				if (ImGui::Button("Generate")) {
					instances.Resize((uint32)(m_BatchDensity.X * m_BatchDensity.Y), Math::FTransform::IDENTITY);
					const Math::FVector2 delta{ (m_BatchEndPos.X - m_BatchStartPos.X) / (float)m_BatchDensity.X, (m_BatchEndPos.Y - m_BatchStartPos.Y) / (float)m_BatchDensity.Y };
					for (int dy = 0; dy < m_BatchDensity.Y; ++dy) {
						for (int dx = 0; dx < m_BatchDensity.X; ++dx) {
							Math::FVector2 pos = m_BatchStartPos + delta * Math::FVector2{ (float)dx, (float)dy };
							if (m_RandomXZ) {
								pos += Math::FVector2{ Util::RandomF(0.0f, delta.X), Util::RandomF(0.0f, delta.Y) };
							}
							instances[dy * m_BatchDensity.X + dx].Position = { pos.X, m_BatchUniformHeight, pos.Y };
						}
					}
				}
				ImGui::TreePop();
			}
			if (instances.Size()) {
				ImGui::SliderInt("Instance Edit Index", &m_InstanceEditIndex, 0, (int)instances.Size() - 1);
				Math::FTransform& transform = instances[m_InstanceEditIndex];
				ImGui::DragFloat3("Position", transform.Position.Data(), 0.1f);
				Math::FVector3 euler = transform.Rotation.ToEuler();
				if (ImGui::DragFloat3("Rotation", euler.Data(), 0.1f)) {
					transform.Rotation = Math::FQuaternion::Euler(euler);
				}
				ImGui::DragFloat3("Scale", transform.Scale.Data(), 0.1f, 0.001f, 1000.0f);
			}
			if (ImGui::Button("Save")) {
				if (Asset::AssetLoader::SaveProjectAsset(&m_Asset, m_PathStr.c_str())) {
					LOG_INFO("Instance data saved: %s", m_PathStr.c_str());
				}
			}
		}
	};

	class MaterialTemplateEditor : public Editor::EditorWndBase {
	public:
		MaterialTemplateEditor(const XString& pathStr) : EditorWndBase("Material Template Editor", ImGuiWindowFlags_NoDocking),
		m_PathStr(pathStr),
		m_FistDisplay(true),
		m_PreviewInScene(false) {
			AutoDelete();
			Asset::AssetLoader::LoadProjectAsset(&m_Asset, m_PathStr.c_str());
		}
		void WndContent() override {
			ImGui::TextUnformatted(m_PathStr.c_str());
			if (ImGui::Button("Save")) {
				Object::MaterialMgr::Instance()->ReloadMaterialTemplate(m_PathStr, m_Asset);
				if (Asset::AssetLoader::SaveProjectAsset(&m_Asset, m_PathStr.c_str())) {
					LOG_INFO("Material template saved: %s", m_PathStr.c_str());
				}
			}
			ImGui::SameLine();
			ImGui::Checkbox("Preview In Scene", &m_PreviewInScene);

			bool bParamTypeDirty = false;
			bool bParamDirty = false;
			constexpr float itemWidth = 128.0f;
			int uniqueID = 0;
			// material params
			ImNodes::PushStyleVar(ImNodesStyleVar_PinCircleRadius, 4.0f);
			ImNodes::BeginNodeEditor();
			const int paramSize = (int)m_Asset.Parameters.Size();
			for(int i=0; i<paramSize; ++i) {
				auto& param = m_Asset.Parameters[i];
				const uint32 paramOffset = param.Offset;
				const Asset::EMaterialParamType paramType = param.ParamType;
				ImNodes::BeginNode(i);
				ImGui::TextUnformatted(param.Name.c_str());
				ImGui::PushID(uniqueID++);
				ImGui::SameLine();
				ImGui::SetNextItemWidth(itemWidth);
				int iParamType = (int)paramType;
				if (ImGui::Combo("##", &iParamType, "Scalar\0Vector\0Texture")) {
					bParamTypeDirty = true;
					param.ParamType = (Asset::EMaterialParamType)iParamType;
					param.Offset = INVALID_INDEX;
				}
				ImGui::PopID();

				switch (paramType) {
				case Asset::EMaterialParamType::Scalar: {
					ImGui::PushID(uniqueID++);
					ImGui::SetNextItemWidth(itemWidth);
					bParamDirty |= ImGui::DragFloat("##", &m_Asset.Scalars[paramOffset], 0.01f, 0.0f, 1.0f);
					ImGui::PopID();
				}break;
				case Asset::EMaterialParamType::Vector: {
					ImGui::PushID(uniqueID++);
					ImGui::SetNextItemWidth(itemWidth * 2);
					bParamDirty |= ImGui::ColorEdit4("##", m_Asset.Vectors[paramOffset].Data());
					ImGui::PopID();
				}break;
				case Asset::EMaterialParamType::Texture: {
					auto& texInfo = m_Asset.Textures[paramOffset];
					XString& texRef = texInfo.Texture;
					ImGui::PushID(uniqueID++);
					ImGui::SetNextItemWidth(itemWidth * 2);
					bParamDirty |= ImGui::DraggableFileItemAssets("##", texRef, "*.texture");
					ImGui::PopID();
					// sampler
					Object::MaterialSamplerCode samplerCode = texInfo.SamplerCode;
					int filter = (int)samplerCode.Filter;
					int addressMode = (int)samplerCode.AddressMode;

					ImGui::PushID(uniqueID++);
					ImGui::SetNextItemWidth(itemWidth);
					bool samplerDirty = ImGui::Combo("Filter", &filter, "Point\0Bilinear\0AnisotropicPoint\0AnisotropicLinear");
					ImGui::PopID();

					ImGui::PushID(uniqueID++);
					ImGui::SetNextItemWidth(itemWidth);
					samplerDirty |= ImGui::Combo("AddressMode", &addressMode, "Warp\0Clamp\0Mirror\0Border");
					ImGui::PopID();
					if (samplerDirty) {
						samplerCode.Filter = (ESamplerFilter)filter;
						samplerCode.AddressMode = (ESamplerAddressMode)addressMode;
						texInfo.SamplerCode = samplerCode.Code;
					}
					bParamDirty |= samplerDirty;
				}break;
				}
				ImNodes::BeginOutputAttribute(i);
				ImNodes::EndOutputAttribute();
				ImNodes::EndNode();
			}

			// display output node
			TArray<TPair<int, int>> links;
			links.Reserve(paramSize);
			ImNodes::BeginNode(paramSize);
			ImGui::TextUnformatted("Output");
			for(int i=0; i< paramSize; ++i) {
				const auto& param = m_Asset.Parameters[i];
				ImNodes::BeginInputAttribute(paramSize + i);
				ImGui::TextUnformatted(param.Name.c_str());
				ImNodes::EndInputAttribute();
				links.PushBack({i, paramSize + i});
			}
			ImNodes::EndNode();
			// align node pos when first tick
			if(m_FistDisplay) {
				constexpr float xStart=64.0f, yStart = 64.0f;
				float y = yStart;
				float maxWidth = 0.0f;
				constexpr float nodeSpace = 16.0f;
				for(int i=0; i< paramSize; ++i) {
					ImNodes::SetNodeGridSpacePos(i, { xStart, y });
					ImVec2 nodeSize = ImNodes::GetNodeDimensions(i);
					y += nodeSize.y + nodeSpace;
					maxWidth = Math::Max(nodeSize.x, maxWidth);
				}
				ImNodes::SetNodeGridSpacePos(paramSize, { xStart + maxWidth + nodeSpace, (yStart + y) * 0.5f });
				m_FistDisplay = false;
			}
			// links
			for(auto link: links) {
				ImNodes::Link(uniqueID++, link.first, link.second);
			}
			ImNodes::EndNodeEditor();
			ImNodes::PopStyleVar();

			if(bParamTypeDirty) {
				RebuildParams();
			}
			bParamDirty |= bParamTypeDirty;
			if(bParamDirty && m_PreviewInScene) {
				Object::MaterialMgr::Instance()->ReloadMaterialTemplate(m_PathStr, m_Asset);
			}
		}
	private:
		XString m_PathStr;
		Asset::MaterialTemplateAsset m_Asset;
		bool m_FistDisplay;
		bool m_PreviewInScene;
		void RebuildParams() {
			TArray<float> scalars;
			TArray<Math::FVector4> vectors;
			TArray<Asset::MaterialTemplateAsset::TextureParameter> textures;
			// parse texture and vectors
			for(auto& param: m_Asset.Parameters) {
				if(Asset::EMaterialParamType::Scalar == param.ParamType) {
					const uint32 newOffset = scalars.Size();
					scalars.PushBack(0.0f);
					if(INVALID_INDEX != param.Offset) {
						scalars[newOffset] = m_Asset.Scalars[param.Offset];
					}
					param.Offset = newOffset;
				}
				else if(Asset::EMaterialParamType::Texture == param.ParamType) {
					const uint32 newOffset = textures.Size();
					textures.PushBack({"", 0u});
					if(INVALID_INDEX != param.Offset) {
						textures[newOffset] = m_Asset.Textures[param.Offset];
					}
					param.Offset = newOffset;
				}
				else if (Asset::EMaterialParamType::Vector == param.ParamType) {
					const uint32 newOffset = vectors.Size();
					vectors.PushBack(Math::FVector4::ZERO);
					if(INVALID_INDEX != param.Offset) {
						vectors[newOffset] = m_Asset.Vectors[param.Offset];
					}
					param.Offset = newOffset;
				}
			}
			m_Asset.Scalars.Swap(scalars);
			m_Asset.Vectors.Swap(vectors);
			m_Asset.Textures.Swap(textures);
		}
	};

	class MaterialInstanceEditor: public Editor::EditorWndBase {
	public:
		MaterialInstanceEditor(const XString& pathStr) : EditorWndBase("Material Editor", ImGuiWindowFlags_NoDocking), m_PathStr(pathStr), m_PreviewInScene(false){
			AutoDelete();
			Asset::AssetLoader::LoadProjectAsset(&m_Asset, m_PathStr.c_str());
			Asset::AssetLoader::LoadProjectAsset(&m_ParentAsset, m_Asset.Template.c_str());
			// verify parameters
			if(!m_Asset.CheckAndFixParameters(m_ParentAsset)) {
				LOG_WARNING("Material instance \"%s\" dose not match its template, fixed.", m_PathStr.c_str());
			}
		}
		void WndContent() override {
			ImGui::TextUnformatted(m_PathStr.c_str());
			ImGui::Checkbox("Preview in Scene", &m_PreviewInScene);
			bool isDirty = false;
			isDirty |= ImGui::DraggableFileItemAssets("Template", m_Asset.Template, "*.matt");
			for(auto& param: m_ParentAsset.Parameters) {
				const char* nameStr = param.Name.c_str();
				switch (param.ParamType) {
				case Asset::EMaterialParamType::Scalar: {
					isDirty |= ImGui::DragFloat(nameStr, &m_Asset.ScalarOverrides[param.Offset], 0.01f, 0.0f, 1.0f);
				}break;
				case Asset::EMaterialParamType::Vector: {
					Math::FVector4& vecRef = m_Asset.VectorOverrides[param.Offset];
					isDirty |= ImGui::ColorEdit4(nameStr, vecRef.Data());
				}break;
				case Asset::EMaterialParamType::Texture: {
					XString& texRef = m_Asset.TextureOverrides[param.Offset];
					isDirty |= ImGui::DraggableFileItemAssets(nameStr, texRef, "*.texture");
				}break;
				}
			}
			isDirty &= m_PreviewInScene;
			if (ImGui::Button("Save")) {
				isDirty = true;
				if (Asset::AssetLoader::SaveProjectAsset(&m_Asset, m_PathStr.c_str())) {
					LOG_INFO("Material instance saved: %s", m_PathStr.c_str());
				}
			}
			if(isDirty) {
				Object::MaterialMgr::Instance()->ReloadMaterialInstance(m_PathStr, m_Asset);
			}
		}
	private:
		XString m_PathStr;
		Asset::MaterialAsset m_Asset;
		Asset::MaterialTemplateAsset m_ParentAsset;
		bool m_PreviewInScene;
	};
}

namespace Editor {

	inline XString GenerateUniqueAssetName(FolderNode* folderNode, const char* ext) {
		uint32 identityNum = 0;
		const File::FPath dirFullPath = folderNode->GetFullPath();
		File::FPath fileFullPath = dirFullPath;
		XString fileName = "NewAsset";
		fileFullPath.append(fileName).replace_extension(ext);
		while (File::Exist(fileFullPath)) {
			fileName = StringFormat("NewAsset%u", identityNum++);
			fileFullPath = dirFullPath;
			fileFullPath.append(fileName).replace_extension(ext);
		}
		File::FPath filePath = folderNode->GetPath();
		filePath.append(fileName).replace_extension(ext);
		return filePath.string();
	}

	FolderAssetView::FolderAssetView(FolderNode* node): m_Node(node) {
		m_ID = m_Node->GetID();
	}

	const XString& FolderAssetView::GetName() {
		return m_Node->GetName();
	}

	void FolderAssetView::Rename(const char* newName) {
		m_Node->Rename(newName);
	}

	bool FolderAssetView::IsFolder() {
		return true;
	}

	void FolderAssetView::Open() {
	}

	void FolderAssetView::Save() {
	}

	void FolderAssetView::OnDrag() {
		//ImGui::SetDragDropPayload("Folder", m_Node, sizeof(FolderNode));
	}

	FileAssetView::FileAssetView(FileNode* node): m_Node(node) {
		m_ID = m_Node->GetID();
	}

	const XString& FileAssetView::GetName() {
		return m_Node->GetName();
	}

	void FileAssetView::Rename(const char* newName) {
		m_Node->RenameWithoutExt(newName);
	}

	bool FileAssetView::IsFolder() {
		return false;
	}

	void FileAssetView::OnDrag() {
		ImGui::SetDragDropPayload("File", m_Node, sizeof(FileNode));
	}

	void MeshAssetView::Open() {
		EditorUIMgr::Instance()->AddWindow(TUniquePtr(new MeshViewer(m_Node->GetPathStr())));
	}

	void TextureAssetView::Open() {
		EditorUIMgr::Instance()->AddWindow(TUniquePtr(new TextureViewer(m_Node->GetPathStr())));
	}

	void LevelAssetView::Open() {
		EditorLevelMgr::Instance()->LoadLevel(m_Node->GetPathStr().c_str());
	}

	NodeID LevelAssetView::NewDefault(uint32 folderNode) {
		FolderNode* folder = Editor::ProjectAssetMgr::Instance()->GetFolderNode(folderNode);
		XString filePathStr = GenerateUniqueAssetName(folder, ".level");
		Editor::EditorLevel level(nullptr);
		Editor::EditorLevel::InitDefault(level);
		if(level.SaveFile(filePathStr.c_str())) {
			return Editor::ProjectAssetMgr::Instance()->CreateFile(filePathStr, folderNode);
		}
		return INVALID_NODE;
	}

	void InstanceDataAssetView::Open() {
		EditorUIMgr::Instance()->AddWindow(TUniquePtr(new InstanceDataViewer(m_Node->GetPathStr())));
	}

	NodeID InstanceDataAssetView::NewDefault(uint32 folderNode) {
		FolderNode* folder = Editor::ProjectAssetMgr::Instance()->GetFolderNode(folderNode);
		XString filePathStr = GenerateUniqueAssetName(folder, ".instd");
		Asset::InstanceDataAsset asset;
		Asset::InstanceDataAsset::InitDefault(asset);
		if(Asset::AssetLoader::SaveProjectAsset(&asset, filePathStr.c_str())) {
			return Editor::ProjectAssetMgr::Instance()->CreateFile(filePathStr, folderNode);
		}
		return INVALID_NODE;
	}

	void MaterialTemplateAssetView::Open() {
		EditorUIMgr::Instance()->AddWindow(TUniquePtr(new MaterialTemplateEditor(m_Node->GetPathStr())));
	}

	NodeID MaterialTemplateAssetView::NewDefault(uint32 folderNode) {
		FolderNode* folder = Editor::ProjectAssetMgr::Instance()->GetFolderNode(folderNode);
		XString filePathStr = GenerateUniqueAssetName(folder, ".matt");
		Asset::MaterialTemplateAsset asset;
		Asset::MaterialTemplateAsset::InitDefault(asset);
		if (Asset::AssetLoader::SaveProjectAsset(&asset, filePathStr.c_str())) {
			return Editor::ProjectAssetMgr::Instance()->CreateFile(filePathStr, folderNode);
		}
		return INVALID_NODE;
	}

	void MaterialInstanceAssetView::Open() {
		EditorUIMgr::Instance()->AddWindow(TUniquePtr(new MaterialInstanceEditor(m_Node->GetPathStr())));
	}

	TUniquePtr<AssetViewBase> CreateAssetView(FileNode* node) {
		const XString& ext = node->GetPath().extension().string();
		if (ext == ".mesh") {
			return TUniquePtr(new MeshAssetView(node));
		}
		if (ext == ".texture") {
			return TUniquePtr(new TextureAssetView(node));
		}
		if (ext == ".level") {
			return TUniquePtr(new LevelAssetView(node));
		}
		if (ext == ".instd") {
			return TUniquePtr(new InstanceDataAssetView(node));
		}
		if (ext == ".matt") {
			return TUniquePtr(new MaterialTemplateAssetView(node));
		}
		if (ext == ".mati") {
			return TUniquePtr(new MaterialInstanceAssetView(node));
		}
		return TUniquePtr(new FileAssetView(node));
	}
}
