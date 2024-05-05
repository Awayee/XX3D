#pragma once
#include "RHI/Public/ImGuiRHI.h"

class ImGuiVulkan: public ImGuiRHI {
public:
	explicit ImGuiVulkan(RHIRenderPass* pass);
	~ImGuiVulkan() override;
	void FrameBegin() override;
	void FrameEnd() override;
	void RenderDrawData(RHICommandBuffer* cmd) override;
	void ImGuiCreateFontsTexture(RHICommandBuffer* cmd) override;
	void ImGuiDestroyFontUploadObjects() override;
public:
	ImGuiContext* m_Context{ nullptr };
	bool m_FrameBegin{ false };
};