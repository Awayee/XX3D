#include "Engine/Public/Engine.h"
#include "System/Public/Config.h"
#include "Window/Public/EngineWIndow.h"
#include "RHI/Public/RHI.h"
#include "Render/Public/DefaultResource.h"
#include "Render/Public/Renderer.h"
#include "System/Public/FrameCounter.h"

namespace Engine {

	XXEngine* s_RunningEngine{ nullptr };

	XXEngine::XXEngine(): m_Running(false) {
		ASSERT(!s_RunningEngine, "Engine allows only one instance!");
		ConfigManager::Initialize();
		EngineWindow::Initialize();
		RHI::Initialize();
		Render::DefaultResources::Initialize();
		Render::RendererMgr::Initialize();
		s_RunningEngine = this;
	}

	XXEngine::~XXEngine() {
		// wait renderer
		Render::RendererMgr::Instance()->WaitQueue();
		Render::RendererMgr::Release();
		Render::DefaultResources::Release();
		RHI::Release();
		EngineWindow::Release();
		s_RunningEngine = nullptr;
	}

	void XXEngine::Update() {
		RHI::Instance()->BeginFrame();// RHI Update must run at the beginning.
		EngineWindow::Instance()->Update();
		Render::RendererMgr::Instance()->Update();
		FrameCounter::Update();// Frame counter ticks at last.
	}

	void XXEngine::Run() {
		m_Running = true;
		while(m_Running) {
			Update();
		}
	}

	void XXEngine::ShutDown() {
		if(s_RunningEngine) {
			s_RunningEngine->m_Running = false;
		}
	}
}
