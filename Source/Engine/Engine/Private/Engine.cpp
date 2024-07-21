#include "Engine/Public/Engine.h"
#include "Resource/Public/Config.h"
#include "Window/Public/EngineWIndow.h"
#include "RHI/Public/RHI.h"
#include "Render/Public/RenderModuleInterface.h"

namespace Engine {

	XXEngine* s_RunningEngine{ nullptr };

	XXEngine::XXEngine(): m_Running(false) {
		ASSERT(!s_RunningEngine, "Engine allows only one instance!");
		ConfigManager::Initialize();
		EngineWindow::Initialize();
		RHI::Initialize();
		Render::Initialize();
		s_RunningEngine = this;
	}

	XXEngine::~XXEngine() {
		RHI::Release();
		EngineWindow::Release();
		Render::Release();
		s_RunningEngine = nullptr;
	}

	void XXEngine::Update() {
		RHI::Instance()->Update();// RHI Update must run at the beginning.
		EngineWindow::Instance()->Update();
		Render::Update();
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
