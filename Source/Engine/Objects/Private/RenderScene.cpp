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
        auto windowSize = Engine::EngineWindow::Instance()->GetWindowSize();
        m_Renderer.SetRenderArea({ 0, 0, windowSize.w, windowSize.h });
    }

    RenderScene::~RenderScene() {
        m_Renderer.WaitAllFence();
    }

    void RenderScene::Update() {
        // draw call
        ResetSceneDrawCall();
        SystemUpdate();
        // Render
        m_Renderer.Execute(&m_DrawCallContext);
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

        SceneData sceneData;
        sceneData.LightUbo.LightDir = m_DirectionalLight->GetLightDir();
        sceneData.LightUbo.LightColor = m_DirectionalLight->GetLightColor();
        sceneData.CameraUbo.View = m_Camera->GetViewMatrix();
        sceneData.CameraUbo.Proj = m_Camera->GetProjectMatrix();
        sceneData.CameraUbo.VP = m_Camera->GetViewProjectMatrix();
        sceneData.CameraUbo.InvVP = m_Camera->GetInvViewProjectMatrix();
        sceneData.CameraUbo.CamPos = m_Camera->GetView().Eye;
        m_UniformBuffer->UpdateData(&sceneData, sizeof(SceneData), 0);
        m_DrawCallContext.PushDrawCall(Render::EDrawCallQueueType::BasePass, [this](RHICommandBuffer* cmd) {
            cmd->SetShaderParameter(0, 0, RHIShaderParam::UniformBuffer(m_UniformBuffer));
        });
    }

    void RenderScene::CreateResources() {
        m_DirectionalLight.Reset(new DirectionalLight);
        m_DirectionalLight->SetDir({ -1, -1, -1 });
        auto size = RHI::Instance()->GetViewport()->GetSize();
        m_Camera.Reset(new Camera(EProjectiveType::Perspective, (float)size.w / (float)size.h, 0.1f, 1000.0f, Math::Deg2Rad * 75.0f));
        m_Camera->SetView({ 0, 4, -4 }, { 0, 2, 0 }, { 0, 1, 0 });
        m_UniformBuffer = RHI::Instance()->CreateBuffer({ EBufferFlagBit::BUFFER_FLAG_UNIFORM, sizeof(LightUBO) + sizeof(CameraUBO), 0 });
        m_UniformBuffer->SetName("Scene_Uniform");
    }
}
