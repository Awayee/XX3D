#include "Runtime/Public/Runtime.h"
#include "RHI/Public/ImGuiRHI.h"
#include "ClientCode/Public/RuntimeLevel.h"
#include "Render/Public/Renderer.h"

namespace Runtime {

	class RuntimeSceneRenderer: public Render::ISceneRenderer {
	public:
		void Render(Render::RenderGraph& rg, Render::RGTextureNode* backBufferNode) override {
			// render DefaultScene to back buffer
			if (Object::RenderScene* defaultScene = Object::RenderScene::GetDefaultScene()) {
				// push ImGui draw call to deferred lighting pass
				defaultScene->GetDrawCallContext().PushDrawCall(Render::EDrawCallQueueType::DeferredLighting,
					[](RHICommandBuffer* cmd) {ImGuiRHI::Instance()->RenderDrawData(cmd); });
				defaultScene->Render(rg, backBufferNode);
			}
		}
	};

	XXRuntime::XXRuntime(): XXEngine() {
		Render::Renderer::Instance()->SetSceneRenderer<RuntimeSceneRenderer>();
		ImGuiRHI::Initialize(nullptr);
		RuntimeLevelMgr::Initialize();
	}

	XXRuntime::~XXRuntime() {
		ImGuiRHI::Release();
		RuntimeLevelMgr::Release();
	}

	void XXRuntime::Update() {
		ImGuiRHI::Instance()->FrameBegin();
		ImGuiRHI::Instance()->FrameEnd();
		XXEngine::Update();
	}
}