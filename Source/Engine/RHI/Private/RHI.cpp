#include "RHI/Public/RHI.h"
#include "VulkanRHI/VulkanRHI.h"
#include "System/Public/EngineConfig.h"
#include "Core/Public/Log.h"
#include "Core/Public/Concurrency.h"
#include "Window/Public/EngineWindow.h"

TUniquePtr<RHI> RHI::s_Instance{ nullptr };

RHI* RHI::Instance() {
	return s_Instance.Get();
}
void RHI::Initialize() {
	ASSERT(!s_Instance, "");
	Engine::ERHIType rhiType = Engine::ConfigManager::GetData().RHIType;

	RHIInitDesc desc;
	desc.AppName = PROJECT_NAME;
#ifdef _DEBUG
	desc.EnableDebug = true;
#else
	desc.EnableDebug = false;
#endif
	desc.WindowHandle = Engine::EngineWindow::Instance()->GetWindowHandle();
	desc.WindowSize = Engine::EngineWindow::Instance()->GetWindowSize();

	switch(rhiType) {
	case Engine::ERHIType::Vulkan:
		s_Instance.Reset(new VulkanRHI(desc));
		break;
	case Engine::ERHIType::DX12:
	case Engine::ERHIType::DX11:
	case Engine::ERHIType::OpenGL:
	case Engine::ERHIType::Invalid:
		ASSERT(0, "Failed to initialize RHI!");
	}
}

void RHI::Release() {
	s_Instance.Reset();
}
