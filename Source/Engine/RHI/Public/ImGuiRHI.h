#pragma once
#include "RHI.h"
#include "Core/Public/TUniquePtr.h"
#include <imgui.h>

class ImGuiRHI{
public:
	static ImGuiRHI* Instance();
	static void Initialize();
	static void Release();
	virtual ~ImGuiRHI() {}
	virtual void FrameBegin() = 0;
	virtual void FrameEnd() = 0;
	virtual void RenderDrawData(RHICommandBuffer* cmd) = 0;
	virtual void ImGuiCreateFontsTexture(RHICommandBuffer* cmd) = 0;
	virtual void ImGuiDestroyFontUploadObjects() = 0;
private:
	static TUniquePtr<ImGuiRHI> s_Instance;
};
