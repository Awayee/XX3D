#pragma once
#include "RHI/Public/ImGuiRHI.h"

class VulkanImGui: public ImGuiRHI {
public:
	VulkanImGui(void(*configInitializer)());
	~VulkanImGui() override;
	void FrameBegin() override;
	void FrameEnd() override;
	void RenderDrawData(RHICommandBuffer* cmd) override;
	ImTextureID RegisterImGuiTexture(RHITexture* texture, RHISampler* sampler) override;
	void RemoveImGuiTexture(ImTextureID textureID) override;
	void ImGuiCreateFontsTexture(RHICommandBuffer* cmd) override;
	void ImGuiDestroyFontUploadObjects() override;
public:
	ImGuiContext* m_Context{ nullptr };
	bool m_IsInitialized = false;
	bool m_FrameBegin{ false };
};