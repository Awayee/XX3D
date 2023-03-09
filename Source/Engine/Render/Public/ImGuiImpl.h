#pragma once
#include "RHI/Public/RHI.h"
#include "imgui.h"
#include "imgui_internal.h"

namespace Engine {
	void ImGuiInitialize(RHI::RRenderPass* pass, uint32 subpass);
	void ImGuiRelease();
	void ImGuiNewFrame();
	void ImGuiEndFrame();
	void ImGuiRenderDrawData(RHI::RCommandBuffer* cmd);
	void ImGuiCreateFontsTexture(RHI::RCommandBuffer* cmd);
	void ImGuiDestroyFontUploadObjects();
}