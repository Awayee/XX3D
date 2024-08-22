#include "System/Public/EngineConfig.h"
#include "Core/Public/Concurrency.h"
#include "Objects/Public/RenderScene.h"
#include "Objects/Public/Camera.h"
#include "Objects/Public/Light/DirectionalLight.h"
#include "Window/Public/EngineWindow.h"

namespace Object {

    TUniquePtr<RenderScene> RenderScene::s_Default;
    TArray<Func<void(RenderScene*)>> RenderScene::s_RegisterSystems;

    struct LightUBO {
        Math::FVector3 LightDir; float _padding;
        Math::FVector3 LightColor; float _padding1;
    };

    struct CameraUBO {
        Math::FMatrix4x4 View;
        Math::FMatrix4x4 Proj;
        Math::FMatrix4x4 VP;
        Math::FMatrix4x4 InvVP;
        Math::FVector3 CamPos;
    };

    struct SceneData {
        LightUBO LightUbo;
        CameraUBO CameraUbo;
    };

    RenderScene::RenderScene() {
        CreateResources();
        // register systems
        for(auto& func: s_RegisterSystems) {
            func(this);
        }
    }

    RenderScene::~RenderScene() {
    }

    void RenderScene::Update() {
        // draw call
        ResetSceneDrawCall();
        SystemUpdate();
        // Render
        m_Renderer.Execute(&m_DrawCallContext);
    }

    void RenderScene::SetViewport(const Rect& viewport) {
        m_Camera->SetAspect((float)viewport.w / (float)viewport.h);
        m_Viewport = { (float)viewport.x, (float)viewport.y, (float)viewport.w, (float)viewport.h };
        m_Scissor = viewport;
    }

    RenderScene* RenderScene::GetDefaultScene() {
        return s_Default.Get();
    }

    void RenderScene::Initialize() {
        s_Default.Reset(new RenderScene());
    }

    void RenderScene::Release() {
        s_Default.Reset();
    }

    void RenderScene::Tick() {
        s_Default->Update();
    }

    void RenderScene::ResetSceneDrawCall() {
        m_DrawCallContext.Reset();

		CameraUBO cameraUBO;
		cameraUBO.View = m_Camera->GetViewMatrix();
		cameraUBO.Proj = m_Camera->GetProjectMatrix();
		cameraUBO.VP = m_Camera->GetViewProjectMatrix();
		cameraUBO.InvVP = m_Camera->GetInvViewProjectMatrix();
		cameraUBO.CamPos = m_Camera->GetView().Eye;
        m_CameraUniform->UpdateData(&cameraUBO, sizeof(SceneData), 0);
        LightUBO lightUBO;
        lightUBO.LightDir = m_DirectionalLight->GetLightDir();
        lightUBO.LightColor = m_DirectionalLight->GetLightColor();
        m_LightUniform->UpdateData(&lightUBO, sizeof(LightUBO), 0);
        m_DrawCallContext.PushDrawCall(Render::EDrawCallQueueType::BasePass, [this](RHICommandBuffer* cmd) {
            cmd->SetShaderParam(0, 0, RHIShaderParam::UniformBuffer(m_CameraUniform));
            cmd->SetViewport(m_Viewport, 0.0f, 1.0f);
            cmd->SetScissor(m_Scissor);
        });
        m_DrawCallContext.PushDrawCall(Render::EDrawCallQueueType::DeferredLighting, [this](RHICommandBuffer* cmd) {
            cmd->SetShaderParam(0, 0, RHIShaderParam::UniformBuffer(m_CameraUniform));
            cmd->SetShaderParam(0, 1, RHIShaderParam::UniformBuffer(m_LightUniform));
        });
    }

    void RenderScene::CreateResources() {
        m_DirectionalLight.Reset(new DirectionalLight);
        m_DirectionalLight->SetDir({ -1, -1, -1 });
        auto size = RHI::Instance()->GetViewport()->GetSize();
        m_Camera.Reset(new Camera(EProjectiveType::Perspective, (float)size.w / (float)size.h, 0.1f, 1000.0f, Math::Deg2Rad * 75.0f));
        m_Camera->SetView({ 0, 4, -4 }, { 0, 2, 0 }, { 0, 1, 0 });
        m_CameraUniform = RHI::Instance()->CreateBuffer({ EBufferFlagBit::BUFFER_FLAG_UNIFORM, sizeof(CameraUBO), 0 });
        m_LightUniform = RHI::Instance()->CreateBuffer({ EBufferFlagBit::BUFFER_FLAG_UNIFORM, sizeof(LightUBO), 0 });
    }
}
