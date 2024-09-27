#include "Runtime/Public/Runtime.h"
#include "ClientCode/Public/RuntimeLevel.h"
#include "ClientCode/Public/RuntimeUI.h"
#include "RHI/Public/RHIImGui.h"
#include "Render/Public/Renderer.h"

namespace Runtime {

	class RuntimeSceneRenderer: public Render::ISceneRenderer {
	public:
		void Render(Render::RenderGraph& rg, Render::RGTextureNode* backBufferNode) override {
			// render DefaultScene to back buffer
			if (Object::RenderScene* defaultScene = Object::RenderScene::GetDefaultScene()) {
				if (RuntimeUIMgr::Instance()->NeedDraw()) {
					Render::RGTextureNode* sceneTargetNode = rg.CopyTextureNode(backBufferNode, "BackBuffer_SceneTarget");
					defaultScene->Render(rg, sceneTargetNode);
					Render::RGRenderNode* imguiNode = rg.CreateRenderNode("ImGui");
					imguiNode->ReadColorTarget(sceneTargetNode, 0, false);
					imguiNode->WriteColorTarget(backBufferNode, 0, false);
					imguiNode->SetTask([this](RHICommandBuffer* cmd) {
						RHIImGui::Instance()->RenderDrawData(cmd);
					});
				}
				else {
					defaultScene->Render(rg, backBufferNode);
				}
			}
		}
	};

	void XXRuntime::PreInitialize() {
		RHI::SetInitConfigBuilder([]()->RHIInitConfig {
			RHIInitConfig cfg{};
			cfg.EnableVSync = false;
			return cfg;
		});
	}

	XXRuntime::XXRuntime(): XXEngine() {
		Render::Renderer::Instance()->SetSceneRenderer<RuntimeSceneRenderer>();
		RHIImGui::Initialize(RuntimeUIMgr::InitializeImGuiConfig);
		RuntimeLevelMgr::Initialize();
		RuntimeUIMgr::Initialize();
	}

	XXRuntime::~XXRuntime() {
		RHIImGui::Release();
		RuntimeLevelMgr::Release();
		RuntimeUIMgr::Release();
	}

	void XXRuntime::Update() {
		RuntimeUIMgr::Instance()->Tick();
		XXEngine::Update();
	}
}
