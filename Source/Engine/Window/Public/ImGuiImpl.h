#pragma once
#include "RHI/Public/RHI.h"
#include "imgui.h"
#include "imgui_internal.h"

namespace Engine {
	void ImGuiInitialize(Engine::RRenderPass* pass, uint32 subpass);
	void ImGuiRelease();
	void ImGuiNewFrame();
	void ImGuiEndFrame();
	void ImGuiRenderDrawData(Engine::RHICommandBuffer* cmd);
	void ImGuiCreateFontsTexture(Engine::RHICommandBuffer* cmd);
	void ImGuiDestroyFontUploadObjects();
}