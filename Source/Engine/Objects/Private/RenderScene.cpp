#include "System/Public/EngineConfig.h"
#include "Core/Public/Concurrency.h"
#include "Objects/Public/RenderScene.h"
#include "Objects/Public/Camera.h"
#include "Objects/Public/Light/DirectionalLight.h"
#include "Objects/Public/StaticMesh.h"

namespace Object {

    TUniquePtr<RenderScene> RenderScene::s_Default;

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
        RegisterSystem<Object::MeshRenderSystem>();
        CreateResources();
    }

    RenderScene::~RenderScene() {
    }

    void RenderScene::Update() {
        UpdateUniform();
        ECSScene::Update();
    }

    void RenderScene::GenerateDrawCall(Render::DrawCallQueue& queue) {
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

    void RenderScene::UpdateUniform() {
        SceneData sceneData;
        sceneData.LightUbo.LightDir = m_DirectionalLight->GetLightDir();
        sceneData.LightUbo.LightColor = m_DirectionalLight->GetLightColor();
        sceneData.CameraUbo.View = m_Camera->GetViewMatrix();
        sceneData.CameraUbo.Proj = m_Camera->GetProjectMatrix();
        sceneData.CameraUbo.VP = m_Camera->GetViewProjectMatrix();
        sceneData.CameraUbo.InvVP = m_Camera->GetInvViewProjectMatrix();
        sceneData.CameraUbo.CamPos = m_Camera->GetView().Eye;
        m_UniformBuffer->UpdateData(&sceneData, sizeof(SceneData), 0);
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
