#include "AssetView.h"
#include "UIExtent.h"
#include "Functions/Public/EditorLevel.h"
#include "EditorUI/Public/EditorUIMgr.h"
#include "Render/Public/DefaultResource.h"
#include "Objects/Public/RenderResource.h"
#include "Util/Public/Random.h"
#include "Window/Public/EngineWindow.h"

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
				ImGui::DraggableFileItemAssets(primitiveName.c_str(), primitive.MaterialFile, ".texture");
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
}

namespace Editor {
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

	void InstanceDataAssetView::Open() {
		EditorUIMgr::Instance()->AddWindow(TUniquePtr(new InstanceDataViewer(m_Node->GetPathStr())));
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
		return TUniquePtr(new FileAssetView(node));
	}
}
