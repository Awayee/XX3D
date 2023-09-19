#pragma once
#include "RHI/Public/RHI.h"
#include "imgui.h"
#include "imgui_internal.h"

namespace Engine {
	void ImGuiInitialize(Engine::RRenderPass* pass, uint32 subpass);
	void ImGuiRelease();
	void ImGuiNewFrame();
	void ImGuiEndFrame();
	void ImGuiRenderDrawData(Engine::RCommandBuffer* cmd);
	void ImGuiCreateFontsTexture(Engine::RCommandBuffer* cmd);
	void ImGuiDestroyFontUploadObjects();
}