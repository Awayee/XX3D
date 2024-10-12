#pragma once
#include "RHI/Public/RHIImGui.h"

class VulkanImGui: public RHIImGui {
public:
	VulkanImGui(void(*configInitializer)());
	~VulkanImGui() override;
	void FrameBegin() override;
	void FrameEnd() override;
	void RenderDrawData(RHICommandBuffer* cmd) override;
	ImTextureID RegisterImGuiTexture(RHITexture* texture, RHITextureSubRes subRes, RHISampler* sampler) override;
	void RemoveImGuiTexture(ImTextureID textureID) override;
public:
	ImGuiContext* m_Context{ nullptr };
};