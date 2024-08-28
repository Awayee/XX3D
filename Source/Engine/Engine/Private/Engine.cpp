#include "Engine/Public/Engine.h"
#include "System/Public/EngineConfig.h"
#include "Window/Public/EngineWIndow.h"
#include "RHI/Public/RHI.h"
#include "RHI/Public/ImGuiRHI.h"
#include "Render/Public/DefaultResource.h"
#include "Render/Public/Renderer.h"
#include "Render/Public/GlobalShader.h"
#include "System/Public/FrameCounter.h"
#include "System/Public/Timer.h"
#include "Objects/Public/TextureResource.h"
#include "Objects/Public/StaticMesh.h"
#include "Objects/Public/RenderScene.h"

namespace Engine {

	XXEngine* s_RunningEngine{ nullptr };

	XXEngine::XXEngine(): m_Running(false) {
		ASSERT(!s_RunningEngine, "Multi XXEngine object is Invalid!");
		ConfigManager::Initialize();
		EngineWindow::Initialize();
		RHI::Initialize();
		Render::DefaultResources::Initialize();
		Render::GlobalShaderMap::Initialize();
		Render::Renderer::Initialize();
		Object::TextureResourceMgr::Initialize();
		Object::MeshRenderSystem::Initialize();
		Object::RenderScene::Initialize();
		s_RunningEngine = this;
	}

	XXEngine::~XXEngine() {
		// wait renderer
		Render::Renderer::Release();
		Object::RenderScene::Release();
		Object::TextureResourceMgr::Release();
		Render::GlobalShaderMap::Release();
		Render::DefaultResources::Release();
		ImGuiRHI::Release();
		RHI::Release();
		EngineWindow::Release();
		s_RunningEngine = nullptr;
	}

	void XXEngine::Update() {
		RHI::Instance()->BeginFrame();// RHI Update must run at the beginning.
		EngineWindow::Instance()->Update();
		Object::RenderScene::Tick();
		Render::Renderer::Instance()->Run();
		CTimer::Instance()->Tick();
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
