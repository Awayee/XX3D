#pragma once
#include "RHI.h"
#include "Core/Public/TUniquePtr.h"
#include <imgui.h>

class RHIImGui{
public:
	static RHIImGui* Instance();
	static void Initialize(void(*configInitializer)());
	static void Release();
	virtual ~RHIImGui() {}
	virtual void FrameBegin() = 0;
	virtual void FrameEnd() = 0;
	virtual void RenderDrawData(RHICommandBuffer* cmd) = 0;
	virtual ImTextureID RegisterImGuiTexture(RHITexture* texture, RHISampler* sampler) = 0;
	virtual void RemoveImGuiTexture(ImTextureID textureID) = 0;
private:
	static TUniquePtr<RHIImGui> s_Instance;
};
