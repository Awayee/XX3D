#include "Engine/Public/Engine.h"
#include "System/Public/EngineConfig.h"
#include "Window/Public/EngineWIndow.h"
#include "RHI/Public/RHI.h"
#include "RHI/Public/RHIImGui.h"
#include "Render/Public/DefaultResource.h"
#include "Render/Public/Renderer.h"
#include "Render/Public/GlobalShader.h"
#include "System/Public/FrameCounter.h"
#include "System/Public/Timer.h"
#include "Objects/Public/TextureResource.h"
#include "Objects/Public/StaticMesh.h"
#include "Objects/Public/RenderScene.h"

void TestWindowFunc() {

	Engine::EngineWindow* window = Engine::EngineWindow::Instance();
	// TODO test
	window->RegisterOnCursorPosFunc([](float x, float y) {LOG_DEBUG("OnCursorPos(%f, %f)", x, y); });
	window->RegisterOnKeyFunc([](Engine::EKey key, Engine::EInput input) {LOG_DEBUG("OnKey(%i, %i)", key, input); });
	window->RegisterOnMouseButtonFunc([](Engine::EBtn btn, Engine::EInput input) {LOG_DEBUG("OnMouseButton(%i, %i)", btn, input); });
	window->RegisterOnScrollFunc([](float x, float y) {LOG_DEBUG("OnScroll(%f,%f)", x, y); });
	window->RegisterOnWindowFocusFunc([](bool b) {LOG_DEBUG("OnWindowFocus(%i)", b); });
	window->RegisterOnDropFunc([](int num, const char** path) {
		LOG_DEBUG("OnDrop(%i)", num);
		for (int i = 0; i < num; ++i) {
			LOG_DEBUG("    Path=%s", path[i]);
		}
	});
}

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
		RHIImGui::Release();
		RHI::Release();
		EngineWindow::Release();
		s_RunningEngine = nullptr;
	}

	void XXEngine::Update() {
		RHI::Instance()->BeginFrame();// RHI Update must run at the beginning.
		Object::RenderScene::Tick();
		Render::Renderer::Instance()->Run();
		EngineWindow::Instance()->Update();
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
