#include "Window/Public/Wnd.h"
#include "Core/Public/Time.h"
#include "Engine/Public/EngineContext.h"
#include "Engine/Public/Engine.h"
#include "Core/Public/Defines.h"
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
			RHIInitDesc desc;
			desc.AppName = PROJECT_NAME;
			desc.EnableDebug =
#ifdef _DEBUG
				true;
#else
				false;
#endif
			desc.WindowHandle = context->m_Window->GetWindowHandle();
			desc.RHIType = GetConfig().RHIType;
			RHI::Initialize(desc);
		}
		// init renderer
		//context->m_Renderer.Reset(new Renderer(context->Window()));
		PRINT_DEBUG("Engine Initialized.");
	}
	XXEngine::~XXEngine() {
		auto* context = Context();
		context->m_Renderer.Reset();
		RHI::Instance()->Release();
		Wnd::Instance()->Release();
	}

	bool XXEngine::Tick() {
		if(Context()->Window()->ShouldClose()) {
			return false;
		}

		Context()->Window()->Tick();
		//Context()->Renderer()->Tick();
		return true;
	}
}
