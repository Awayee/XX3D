#pragma once
#include "RHI.h"
#include "Core/Public/TUniquePtr.h"
#include <imgui.h>

class RHIImGui{
public:
	static RHIImGui* Instance();
	static void Initialize(void(*configInitializer)());
	static void Release();
	virtual void FrameBegin() = 0;
	virtual void FrameEnd() = 0;
	virtual void RenderDrawData(RHICommandBuffer* cmd) = 0;
	virtual ImTextureID RegisterImGuiTexture(RHITexture* texture, RHITextureSubRes subRes, RHISampler* sampler) = 0;
	virtual void RemoveImGuiTexture(ImTextureID textureID) = 0;
private:
	friend TDefaultDeleter<RHIImGui>;
	static TUniquePtr<RHIImGui> s_Instance;
	ImGuiContext* m_Context;
protected:
	RHIImGui();
	virtual ~RHIImGui();
};
