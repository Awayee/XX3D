#pragma once
#include "D3D12Util.h"
#include "D3D12Descriptor.h"
#include "RHI/Public/RHIImGui.h"
#include "Core/Public/Container.h"

class D3D12ImGui: public RHIImGui {
public:
	D3D12ImGui(void(*configInitializer)());
	~D3D12ImGui() override;
	void FrameBegin() override;
	void FrameEnd() override;
	void RenderDrawData(RHICommandBuffer* cmd) override;
	ImTextureID RegisterImGuiTexture(RHITexture* texture, RHISampler* sampler) override;
	void RemoveImGuiTexture(ImTextureID textureID) override;
private:
	ImGuiContext* m_Context;
	ID3D12Device* m_Device;
	TUniquePtr<StaticDescriptorAllocator> m_DescriptorAllocator;
	StaticDescriptorHandle m_FontDescriptor;
	TMap<ImTextureID, StaticDescriptorHandle> m_ImTextureDescriptorMap;
};