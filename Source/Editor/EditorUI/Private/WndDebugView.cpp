#include "WndDebugView.h"
#include "WndDebugView.h"
#include "UIExtent.h"
#include "EditorUI/Public/EditorUIMgr.h"
#include "Objects/Public/DirectionalLight.h"
#include "Objects/Public/RenderScene.h"
#include "Render/Public/DefaultResource.h"
#include "Objects/Public/RenderCamera.h"

namespace Editor {

	DebugViewBase::DebugViewBase(): m_CacheTexture(nullptr), m_ArraySize(0), m_MipSize(0), m_ArrayIndex(0), m_MipIndex(0) {
	}

	DebugViewBase::~DebugViewBase() {
		Release();
	}

	void DebugViewBase::Display() {
		RHITexture* tex = GetTexture();
		if (tex != m_CacheTexture) {
			Release();
			if (tex) {
				m_ArraySize = (int)tex->GetDesc().ArraySize;
				m_MipSize = (int)tex->GetDesc().MipSize;
				m_ArrayIndex = Math::Min(m_ArrayIndex, m_MipSize);
				m_MipIndex = Math::Min(m_MipIndex, m_MipSize);
				const ETextureViewFlags viewFlags = GetTextureViewFlags();
				for(int i=0; i<m_ArraySize; ++i) {
					for(int j=0; j<m_MipSize; ++j) {
						RHITextureSubRes subRes{ (uint16)i, 1, (uint8)j, 1, ETextureDimension::Tex2D, viewFlags };
						m_ImGuiIDs.PushBack(RHIImGui::Instance()->RegisterImGuiTexture(tex, subRes, GetSampler()));
					}
				}
			}
			m_CacheTexture = tex;
		}
		if(m_CacheTexture) {
			ImGui::SliderInt("Array Index", &m_ArrayIndex, 0, m_ArraySize - 1);
			ImGui::SliderInt("Mip Index", &m_MipIndex, 0, m_MipSize - 1);
			const float contentWidth = ImGui::GetContentRegionAvail().x;
			float w = (float)m_CacheTexture->GetDesc().Width;
			float h = (float)m_CacheTexture->GetDesc().Height;
			h = contentWidth * h / w;
			w = contentWidth;
			const int subResIndex = m_MipSize * m_ArrayIndex + m_MipIndex;
			ImGui::Image(m_ImGuiIDs[subResIndex], { w, h });
		}
	}

	void DebugViewBase::Release() {
		for (ImTextureID textureID : m_ImGuiIDs) {
			RHIImGui::Instance()->RemoveImGuiTexture(textureID);
		}
		m_ImGuiIDs.Reset();
	}

	class ShadowMapDebugView : public DebugViewBase {
	public:
		using DebugViewBase::DebugViewBase;
		~ShadowMapDebugView() override = default;
		RHITexture* GetTexture() override {
			if(Object::RenderScene* scene = Object::RenderScene::GetDefaultScene()) {
				if(Object::DirectionalLight* light = scene->GetDirectionalLight()) {
					if(light || !light->IsShadowMapValid()) {
						return light->GetShadowMap();
					}
				}
			}
			return nullptr;
		}
		ETextureViewFlags GetTextureViewFlags() override { return ETextureViewFlags::Depth; }
		RHISampler* GetSampler() override {
			return Render::DefaultResources::Instance()->GetDefaultSampler(ESamplerFilter::Bilinear, ESamplerAddressMode::Clamp);
		}
	};


	class HZBDebugView : public DebugViewBase{
	public:
		~HZBDebugView() override = default;
		RHITexture* GetTexture() override {
			if(Object::RenderScene* scene = Object::RenderScene::GetDefaultScene()) {
				if(Object::RenderCamera* camera = scene->GetMainCamera()) {
					if(Render::HZBBuilder* hzbBuilder = camera->GetRenderContext().HZB) {
						return hzbBuilder->GetTexture();
					}
				}
			}
			return nullptr;
		}

		ETextureViewFlags GetTextureViewFlags() override { return ETextureViewFlags::Color; }

		RHISampler* GetSampler() override {
			return Render::DefaultResources::Instance()->GetDefaultSampler(ESamplerFilter::Point, ESamplerAddressMode::Clamp);
		}
	};

	WndDebugView::WndDebugView() :
	EditorWndBase("Debug View", ImGuiWindowFlags_NoScrollbar, EWndUpdateOrder::Order1), m_ViewMode(DebugViewMode::DV_DIRECTIONAL_SHADOW){
		EditorUIMgr::Instance()->AddMenu("Window", m_Name.c_str(), {}, &m_Enable);
		m_DebugViews[DebugViewMode::DV_DIRECTIONAL_SHADOW].Reset(new ShadowMapDebugView());
		m_DebugViews[DebugViewMode::DV_HZB].Reset(new HZBDebugView());
		m_Enable = false;
	}

	WndDebugView::~WndDebugView() {
	}

	void WndDebugView::WndContent() {
		ImGui::Combo("View Mode", &m_ViewMode, "Directional Shadow\0HZB\0");
		m_ViewMode = Math::Min<int>(m_ViewMode, DebugViewMode::DV_COUNT);
		m_DebugViews[m_ViewMode]->Display();
	}
}
