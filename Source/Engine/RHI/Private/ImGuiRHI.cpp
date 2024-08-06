#include "RHI/Public/ImGuiRHI.h"
#include "VulkanRHI/VulkanImGui.h"
#include "System/Public/Config.h"

TUniquePtr<ImGuiRHI> ImGuiRHI::s_Instance;

ImGuiRHI* ImGuiRHI::Instance() {
	return s_Instance.Get();
}

void ImGuiRHI::Initialize(void(*configInitializer)()) {
	const Engine::ERHIType rhiType = Engine::ConfigManager::GetData().RHIType;
	if(Engine::ERHIType::Vulkan == rhiType) {
		s_Instance.Reset(new VulkanImGui(configInitializer));
	}
	else {
		LOG_ERROR("Failed to initialize imgui!");
	}
}

void ImGuiRHI::Release() {
	s_Instance.Reset();
}
