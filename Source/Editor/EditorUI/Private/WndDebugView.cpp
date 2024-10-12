#include "WndDebugView.h"
#include "UIExtent.h"
#include "EditorUI/Public/EditorUIMgr.h"
#include "Objects/Public/DirectionalLight.h"
#include "Objects/Public/RenderScene.h"
#include "Render/Public/DefaultResource.h"

namespace Editor {

	WndDebugView::WndDebugView() :
	EditorWndBase("Debug View", ImGuiWindowFlags_NoScrollbar, EWndUpdateOrder::Order1), m_ViewMode(DV_DIRECTIONAL_SHADOW), m_ShadowMapArrayIdx(0), m_CachedShadowMap(nullptr) {
		EditorUIMgr::Instance()->AddMenu("Window", m_Name, {}, &m_Enable);
		m_Enable = false;
	}

	WndDebugView::~WndDebugView() {
		ReleaseShadowMapIDs();
	}

	void WndDebugView::WndContent() {
		ImGui::Combo("View Mode", &m_ViewMode, "Directional Shadow\0");
		if(DV_DIRECTIONAL_SHADOW == m_ViewMode) {
			ShowShadowMap();
		}
	}

	void WndDebugView::ShowShadowMap() {
		Object::RenderScene* scene = Object::RenderScene::GetDefaultScene();
		if (!scene) {
			return;
		}
		Object::DirectionalLight* light = scene->GetDirectionalLight();
		if (!light) {
			return;
		}
		/* shadow config modified in WndLevelSetting, which has order of 0,
		 * and the order of debug view is1, so we should check if config matches texture, and update ImTextureID */
		if(light->IsShadowMapValid()) {
			RHITexture* shadowMap = light->GetShadowMap();
			if (shadowMap != m_CachedShadowMap || m_ShadowMapImGuiIDs.IsEmpty()) {
				ReleaseShadowMapIDs();
				if (shadowMap) {
					for (uint32 i = 0; i < light->GetCascadeNum(); ++i) {
						RHITextureSubRes subRes{ (uint16)i, 1, 0, 1, ETextureDimension::Tex2D, ETextureViewFlags::Depth };
						auto* sampler = Render::DefaultResources::Instance()->GetDefaultSampler(ESamplerFilter::Bilinear, ESamplerAddressMode::Clamp);
						m_ShadowMapImGuiIDs.PushBack(RHIImGui::Instance()->RegisterImGuiTexture(shadowMap, subRes, sampler));
					}
				}
				m_CachedShadowMap = shadowMap;
			}
			// display textures
			ImGui::SliderInt("Array Index", &m_ShadowMapArrayIdx, 0, (int)m_ShadowMapImGuiIDs.Size()-1);
			const float contentWidth = ImGui::GetContentRegionAvail().x;
			LOG_DEBUG("ContentSize (%f, %f)", contentWidth, ImGui::GetContentRegionAvail().y);
			float w = (float)shadowMap->GetDesc().Width;
			float h = (float)shadowMap->GetDesc().Height;
			h = contentWidth * h / w;
			w = contentWidth;
			ImGui::Image(m_ShadowMapImGuiIDs[m_ShadowMapArrayIdx], { w, h });
		}
		else if (m_ShadowMapImGuiIDs.Size()) {
			ReleaseShadowMapIDs();
			m_CachedShadowMap = nullptr;
		}
	}

	void WndDebugView::ReleaseShadowMapIDs() {
		for (ImTextureID textureID : m_ShadowMapImGuiIDs) {
			RHIImGui::Instance()->RemoveImGuiTexture(textureID);
		}
		m_ShadowMapImGuiIDs.Reset();
	}
}
