#pragma once
#include "RHI/Public/ImGuiRHI.h"

class VulkanImGui: public ImGuiRHI {
public:
	VulkanImGui();
	~VulkanImGui() override;
	void FrameBegin() override;
	void FrameEnd() override;
	void RenderDrawData(RHICommandBuffer* cmd) override;
	void ImGuiCreateFontsTexture(RHICommandBuffer* cmd) override;
	void ImGuiDestroyFontUploadObjects() override;
public:
	ImGuiContext* m_Context{ nullptr };
	bool m_IsInitialized = false;
	bool m_FrameBegin{ false };
};