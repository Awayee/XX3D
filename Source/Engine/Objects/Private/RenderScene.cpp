#include "Resource/Public/Config.h"
#include "Core/Public/Concurrency.h"

#include "Objects/Public/RenderScene.h"
#include "Objects/Public/Camera.h"
#include "Objects/Public/Light/DirectionalLight.h"

namespace Engine {
    RenderObject::RenderObject(RenderScene* scene)
    {
        if (nullptr == scene)scene = RenderScene::GetDefaultScene();
        scene->AddRenderObject(this);
    }
    RenderObject::~RenderObject()
    {
        m_Scene->RemoveRenderObject(this);
        m_Scene = nullptr;
        m_Index = 0u;
    }

    TUniquePtr<RenderScene> RenderScene::s_Default;

    struct SceneUBO {
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
    void RenderScene::UpdateUniform() {
        SceneUBO sceneUbo;
        sceneUbo.LightDir = m_DirectionalLight->GetLightDir();
        sceneUbo.LightColor = m_DirectionalLight->GetLightColor();
        m_LightUniform.UpdateData(&sceneUbo);
        CameraUBO cameraUbo;
        cameraUbo.View = m_Camera->GetViewMatrix();
        cameraUbo.Proj = m_Camera->GetProjectMatrix();
        cameraUbo.VP = m_Camera->GetViewProjectMatrix();
        cameraUbo.InvVP = m_Camera->GetInvViewProjectMatrix();
        cameraUbo.CamPos = m_Camera->GetView().Eye;
        m_CameraUniform.UpdateData(&cameraUbo);
    }

    void RenderScene::CreateResources() {
        m_DirectionalLight.reset(new DirectionalLight);
        m_DirectionalLight->SetDir({-1, -1, -1});
        auto& ext = RHI::Instance()->GetSwapchainExtent();
        m_Camera.reset(new Camera(CAMERA_PERSPECTIVE, (float)ext.w / ext.h, 0.1f, 1000.0f, Math::Deg2Rad * 75.0f));
        m_Camera->SetView({ 0, 4, -4 }, { 0, 2, 0}, { 0, 1, 0 });
    }

    void RenderScene::CreateDescriptorSets() {
        GET_RHI(rhi);

        m_SceneDescs = rhi->AllocateDescriptorSet(DescsMgr::Get(DESCS_SCENE));

        uint32 bufferSize = sizeof(CameraUBO);
        m_CameraUniform.CreateForUniform(bufferSize, nullptr);
        m_SceneDescs->SetUniformBuffer(0, m_CameraUniform.Buffer);

        bufferSize = sizeof(SceneUBO);
        m_LightUniform.CreateForUniform(bufferSize, nullptr);
        m_SceneDescs->SetUniformBuffer(1, m_LightUniform.Buffer);
    }

    RenderScene* RenderScene::GetDefaultScene() {
        if(nullptr == s_Default.get()) {
            static Mutex m;
            MutexLock lock(m);
            if (nullptr == s_Default.get()) {
                s_Default.reset(new RenderScene);
            }
        }
        return s_Default.get();
    }

    RenderScene::RenderScene() {
        CreateResources();
        CreateDescriptorSets();
    }

    RenderScene::~RenderScene() {
        m_LightUniform.Release();
        m_CameraUniform.Release();
        //RHI::Instance()->FreeDescriptorSet(m_SceneDescs);
    }

    void RenderScene::AddRenderObject(RenderObject* obj) {
        obj->m_Scene = this;
        obj->m_Index = m_RenderObjects.Size();
        m_RenderObjects.PushBack(obj);
    }

    void RenderScene::RemoveRenderObject(RenderObject* obj) {
        if(obj->m_Scene != this || RenderObject::INVALID_INDEX == obj->m_Index) {
            return;
        }
        if (!m_RenderObjects.Empty()) {
            m_RenderObjects.Back()->m_Index = obj->m_Index;
            m_RenderObjects.SwapRemoveAt(obj->m_Index);
        }
    }

    void RenderScene::Update() {
        UpdateUniform();
        for(RenderObject* obj: m_RenderObjects) {
            obj->Update();
        }
    }

    void RenderScene::RenderGBuffer(Engine::RCommandBuffer* cmd, Engine::RPipelineLayout* layout) {
        cmd->BindDescriptorSet(layout, m_SceneDescs, 0, Engine::PIPELINE_GRAPHICS);
        for(RenderObject* obj: m_RenderObjects) {
            obj->DrawCall(cmd, layout);
        }
    }
}