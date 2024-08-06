#include "Window/Public/EngineWindow.h"
#include "Core/Public/Concurrency.h"
#include "Core/Public/Log.h"
#include "Core/Public/TUniquePtr.h"
#include "System/Public/Config.h"
#include "Window/Private/WindowGLFW/WindowSystemGLFW.h"
#include "WIndow/Private/WindowWin32/WindowSystemWin32.h"

namespace Engine {

	TUniquePtr<EngineWindow> EngineWindow::s_Instance;

	void EngineWindow::Initialize() {
		const auto& configData = ConfigManager::GetData();
		ERHIType rhiType = configData.RHIType;
		WindowInitInfo windowInfo;
		windowInfo.width = configData.WindowSize.w;
		windowInfo.height = configData.WindowSize.h;
		windowInfo.title = PROJECT_NAME;
		windowInfo.resizeable = true;
		if (ERHIType::Vulkan == rhiType) {
			s_Instance.Reset(new WindowSystemGLFW(windowInfo));
		}
		else if (ERHIType::DX12 == rhiType) {
			s_Instance.Reset(new WindowSystemWin32(windowInfo));
		}
		else {
			ASSERT(0, "can not resolve rhi type");
		}
	}

	void EngineWindow::Release() {
		s_Instance.Reset();
	}

	EngineWindow* EngineWindow::Instance() {
		return s_Instance.Get();
	}
}
