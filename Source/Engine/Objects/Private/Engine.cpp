#include "Window/Public/Wnd.h"
#include "Objects/Public/Renderer.h"
#include "Core/Public/Time.h"
#include "Core/Public/TPtr.h"
#include "Objects/Public/Engine.h"
#include "Objects/Public/EngineContext.h"
#include "Core/Public/macro.h"
#include "Resource/Public/Config.h"

namespace Engine {
	XXEngine::XXEngine() {
		auto* context = Context();
		// init window
		{
			WindowInitInfo windowInfo;
			windowInfo.width = GetConfig().WindowSize.w;
			windowInfo.height = GetConfig().WindowSize.h;
			windowInfo.title = PROJECT_NAME;
			windowInfo.resizeable = true;
			Wnd::Instance()->Initialize(windowInfo);
			context->m_Window = Engine::Wnd::Instance();
		}
		// init rhi
		{
			Engine::RSInitInfo rhiInfo;
			rhiInfo.ApplicationName = PROJECT_NAME;
#ifdef _DEBUG
			rhiInfo.EnableDebug = true;
#else
			rhiInfo.EnableDebug = false
#endif
			rhiInfo.WindowSize = context->m_Window->GetWindowSize();
			rhiInfo.WindowHandle = context->m_Window->GetWindowHandle();
			rhiInfo.MaxFramesInFlight = 3;
			RHI::Instance()->Initialize(&rhiInfo);
		}
		// init renderer
		context->m_Renderer.reset(new Renderer(context->Window()));
		PRINT("Engine Initialized.");
	}
	XXEngine::~XXEngine() {
		auto* context = Context();
		context->m_Renderer.reset();
		RHI::Instance()->Release();
		Wnd::Instance()->Release();
	}

	bool XXEngine::Tick() {
		if(Context()->Window()->ShouldClose()) {
			return false;
		}

		Context()->Window()->Tick();
		Context()->Renderer()->Tick();
		return true;
	}
}
