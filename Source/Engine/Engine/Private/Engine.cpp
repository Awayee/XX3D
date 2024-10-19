#include "Engine/Public/Engine.h"
#include "System/Public/Configuration.h"
#include "Window/Public/EngineWIndow.h"
#include "RHI/Public/RHI.h"
#include "RHI/Public/RHIImGui.h"
#include "Render/Public/DefaultResource.h"
#include "Render/Public/Renderer.h"
#include "Render/Public/GlobalShader.h"
#include "System/Public/Timer.h"
#include "Objects/Public/RenderResource.h"
#include "Objects/Public/RenderScene.h"

namespace Engine {

	XXEngine* s_RunningEngine{ nullptr };

	XXEngine::XXEngine(): m_Running(false) {
		ASSERT(!s_RunningEngine, "Multi XXEngine object is Invalid!");
		EngineConfig::Initialize();
		ProjectConfig::Initialize();
		EngineWindow::Initialize();
		RHI::Initialize();
		Render::DefaultResources::Initialize();
		Render::GlobalShaderMap::Initialize();
		Render::Renderer::Initialize();
		Object::StaticResourceMgr::Initialize();
		Object::RenderScene::Initialize();
		s_RunningEngine = this;
	}

	XXEngine::~XXEngine() {
		// wait renderer
		Render::Renderer::Release();
		Object::RenderScene::Release();
		Object::StaticResourceMgr::Release();
		Render::GlobalShaderMap::Release();
		Render::DefaultResources::Release();
		RHIImGui::Release();
		RHI::Release();
		EngineWindow::Release();
		s_RunningEngine = nullptr;
	}

	void XXEngine::Update() {
		EngineWindow::Instance()->Update();
		RHI::Instance()->BeginFrame();// RHI Update must run at the beginning.
		Object::RenderScene::Tick();
		RHI::Instance()->BeginRendering();
		Render::Renderer::Instance()->Run();
		Timer::Instance().Tick();
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
