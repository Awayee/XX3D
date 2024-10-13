#include "RHI/Public/RHIImGui.h"
#include "VulkanRHI/VulkanImGui.h"
#include "D3D12RHI/D3D12ImGui.h"
#include "System/Public/Configuration.h"

TUniquePtr<RHIImGui> RHIImGui::s_Instance;

RHIImGui* RHIImGui::Instance() {
	return s_Instance.Get();
}

void RHIImGui::Initialize(void(*configInitializer)()) {
	const Engine::ERHIType rhiType = Engine::ProjectConfig::Instance().RHIType;
	if(Engine::ERHIType::Vulkan == rhiType) {
		s_Instance.Reset(new VulkanImGui(configInitializer));
	}
	else if(Engine::ERHIType::D3D12 == rhiType) {
		s_Instance.Reset(new D3D12ImGui(configInitializer));
	}
	else {
		LOG_ERROR("Failed to initialize imgui!");
	}
}

void RHIImGui::Release() {
	s_Instance.Reset();
}
