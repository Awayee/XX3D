#include "RHI/Public/RHI.h"
#include "VulkanRHI/VulkanRHI.h"
#include "D3D12RHI/D3D12RHI.h"
#include "System/Public/EngineConfig.h"
#include "Core/Public/Log.h"
#include "Core/Public/Concurrency.h"
#include "Window/Public/EngineWindow.h"
#include <renderdoc_app.h>
#ifdef _WIN32
#include <Windows.h>
#endif
namespace {
	// load RenderDoc
	#ifdef _WIN32
	XString GetRenderDocPath() {
		HKEY hKey;
		const char* subKey = "SOFTWARE\\Classes\\CLSID\\{5D6BF029-A6BA-417A-8523-120492B1DCE3}\\InprocServer32";
		char dllPath[MAX_PATH];
		DWORD pathSize = sizeof(dllPath);
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
			if (RegQueryValueEx(hKey, NULL, NULL, NULL, (LPBYTE)dllPath, &pathSize) == ERROR_SUCCESS) {
				RegCloseKey(hKey);
				return XString(dllPath);
			}
			RegCloseKey(hKey);
		}
		return "";
	}
	#endif
	RENDERDOC_API_1_6_0* LoadRenderDocAPI() {
		RENDERDOC_API_1_6_0* renderDocAPI = nullptr;
	#ifdef _WIN32
		XString renderDocPath = GetRenderDocPath();
		if (HMODULE mod = LoadLibraryA(renderDocPath.c_str())) {
			pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
			int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void**)&renderDocAPI);
			CHECK(ret == 1);
		}
		else {
			LOG_WARNING("Failed to load renderdoc.");
		}
		return renderDocAPI;
	#endif
	}
}

static RENDERDOC_API_1_6_0* s_RenderDocAPI{ nullptr };

TUniquePtr<RHI> RHI::s_Instance{ nullptr };
RHIInitConfigBuilder RHI::s_InitConfigBuilder{ nullptr };

void RHI::SetInitConfigBuilder(RHIInitConfigBuilder f) {
	s_InitConfigBuilder = f;
}

RHI* RHI::Instance() {
	return s_Instance.Get();
}
void RHI::Initialize() {
	ASSERT(!s_Instance, "");

	// Load RHI Config.
	// Load RenderDoc firstly
	if (Engine::ConfigManager::GetData().EnableRenderDoc) {
		s_RenderDocAPI = LoadRenderDocAPI();
		if (s_RenderDocAPI) {
			s_RenderDocAPI->SetCaptureFilePathTemplate("RenderDocCapture/Capture");
			s_RenderDocAPI->MaskOverlayBits(0, 0);
			LOG_INFO("RenderDoc loaded!");
		}
	}
	const Engine::ERHIType rhiType = Engine::ConfigManager::GetData().RHIType;
	// Create RHI instance.
	const WindowHandle wnd = Engine::EngineWindow::Instance()->GetWindowHandle();
	const USize2D extent = Engine::EngineWindow::Instance()->GetWindowSize();
	const RHIInitConfig cfg = s_InitConfigBuilder ? s_InitConfigBuilder() : RHIInitConfig{};
	switch(rhiType) {
	case Engine::ERHIType::Vulkan:
		s_Instance.Reset(new VulkanRHI(wnd, extent, cfg));
		break;
	case Engine::ERHIType::D3D12:
		s_Instance.Reset(new D3D12RHI(wnd, extent, cfg));
		break;
	default:
		LOG_ERROR("Failed to initialize RHI!");
	}
}

void RHI::Release() {
	s_Instance.Reset();
}

void RHI::CaptureFrame() {
	if(!s_RenderDocAPI) {
		LOG_WARNING("RenderDoc not found!");
		return;
	}
	s_RenderDocAPI->TriggerCapture();
	if (!s_RenderDocAPI->IsTargetControlConnected()) {
		s_RenderDocAPI->LaunchReplayUI(1, nullptr);
	}
}
