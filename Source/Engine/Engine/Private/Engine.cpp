#include "Engine/Public/Engine.h"
#include "System/Public/EngineConfig.h"
#include "Window/Public/EngineWIndow.h"
#include "RHI/Public/RHI.h"
#include "Render/Public/DefaultResource.h"
#include "Render/Public/Renderer.h"
#include "Render/Public/GlobalShader.h"
#include "System/Public/FrameCounter.h"
#include "System/Public/Timer.h"

namespace Engine {

	XXEngine* s_RunningEngine{ nullptr };

	XXEngine::XXEngine(): m_Running(false) {
		ASSERT(!s_RunningEngine, "Engine allows only one instance!");
		ConfigManager::Initialize();
		EngineWindow::Initialize();
		RHI::Initialize();
		Render::DefaultResources::Initialize();
		Render::GlobalShaderMap::Initialize();
		Render::RendererMgr::Initialize();
		s_RunningEngine = this;
	}

	XXEngine::~XXEngine() {
		// wait renderer
		Render::RendererMgr::Instance()->WaitQueue();
		Render::RendererMgr::Release();
		Render::GlobalShaderMap::Release();
		Render::DefaultResources::Release();
		RHI::Release();
		EngineWindow::Release();
		s_RunningEngine = nullptr;
	}

	void XXEngine::Update() {
		RHI::Instance()->BeginFrame();// RHI Update must run at the beginning.
		EngineWindow::Instance()->Update();
		Render::RendererMgr::Instance()->Update();
		CTimer::Instance()->Tick();
		FrameCounter::Update();// Frame counter ticks at last.e
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
