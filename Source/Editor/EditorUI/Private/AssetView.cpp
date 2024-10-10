#include "AssetView.h"
#include "UIExtent.h"
#include "Functions/Public/EditorLevel.h"
#include "EditorUI/Public/EditorUIMgr.h"
#include "Render/Public/DefaultResource.h"
#include "Window/Public/EngineWindow.h"

namespace {
	class TextureViewer : public Editor::EditorWndBase{
	public:
		TextureViewer(const XString& pathStr): EditorWndBase("TextureViewer", ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_HorizontalScrollbar), m_PathStr(pathStr) {
			if(!Asset::AssetLoader::LoadProjectAsset(&m_Asset, m_PathStr.c_str())) {
				return;
			}
			RHITextureDesc desc = RHITextureDesc::Texture2D();
			desc.Format = ERHIFormat::R8G8B8A8_UNORM;
			desc.Width = m_Asset.Width;
			desc.Height = m_Asset.Height;
			desc.Flags = ETextureFlags::SRV | ETextureFlags::CopyDst;
			m_Texture = RHI::Instance()->CreateTexture(desc);
			m_Texture->UpdateData(m_Asset.Pixels.Size(), m_Asset.Pixels.Data());
			auto* sampler = Render::DefaultResources::Instance()->GetDefaultSampler(ESamplerFilter::Bilinear, ESamplerAddressMode::Clamp);
			m_TextureID = RHIImGui::Instance()->RegisterImGuiTexture(m_Texture.Get(), sampler);
			AutoDelete();
			// zoom to fit
			const float w = (float)desc.Width, h = (float)desc.Height;
			constexpr float uniformSize = 512.0f;
			m_Zoom = Math::Min(ZOOM_MAX, uniformSize / Math::Max(w, h));
		}
		~TextureViewer() {
			RHIImGui::Instance()->RemoveImGuiTexture(m_TextureID);
		}
		void WndContent() override {
			const auto& desc = m_Texture->GetDesc();
			const float w = (float)desc.Width, h = (float)desc.Height;
			// zoom
			const float minSize = Math::Min(w, h);
			const float zoomMin = 2.0f / minSize;
			// info
			ImGui::Text("%u x %u", desc.Width, desc.Height);
			ImGui::SliderFloat("Zoom", &m_Zoom, zoomMin, ZOOM_MAX);
			// image
			ImGui::BeginChild("Preview", { 0.0f, 0.0f }, ImGuiChildFlags_Border, ImGuiWindowFlags_HorizontalScrollbar);
			const ImVec2 textureSize{ m_Zoom * w, m_Zoom * h };
			ImGui::Image(m_TextureID, textureSize);
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
				const ImVec2 wndSize = ImGui::GetWindowSize();
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
		ImTextureID m_TextureID;
		float m_Zoom = 512.0f;
		bool m_Dragging = false;
		static constexpr float ZOOM_MAX{ 4.0f };
	};

	class MeshViewer: public Editor::EditorWndBase {
	public:
		MeshViewer(const XString& pathStr): EditorWndBase("MeshViewer", ImGuiWindowFlags_NoDocking), m_PathStr(pathStr) {
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
}

namespace Editor {
	FolderAssetView::FolderAssetView(FolderNode* node): m_Node(node) {
		m_ID = m_Node->GetID();
	}

	const XString& FolderAssetView::GetName() {
		return m_Node->GetName();
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
		return TUniquePtr(new FileAssetView(node));
	}
}
