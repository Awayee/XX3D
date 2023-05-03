#include "Render/Public/RenderScene.h"
#include "Resource/Public/Config.h"
#include "Core/Public/Concurrency.h"

#include "Render/Public/Camera.h"
#include "Render/Public/Light/DirectionalLight.h"

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
        Math::FVector3 Pos; float _padding;
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
        cameraUbo.Pos = m_Camera->GetView().Eye;
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

        uint32 bufferSize = sizeof(SceneUBO);
        m_LightUniform.CreateForUniform(bufferSize, nullptr);
        m_SceneDescs->UpdateUniformBuffer(0, m_LightUniform.Buffer);

        bufferSize = sizeof(CameraUBO);
        m_CameraUniform.CreateForUniform(bufferSize, nullptr);
        m_SceneDescs->UpdateUniformBuffer(1, m_CameraUniform.Buffer);
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
        m_RenderObjects.push_back(obj);
        obj->m_Scene = this;
        obj->m_Index = m_RenderObjects.size();
    }

    void RenderScene::RemoveRenderObject(RenderObject* obj) {
        if (obj->m_Scene != this) return;
        if (0 == obj->m_Index || m_RenderObjects.size() < obj->m_Index)return;

        Swap(m_RenderObjects[obj->m_Index-1], m_RenderObjects.back());
        m_RenderObjects.pop_back();
        if(!m_RenderObjects.empty()) {
            m_RenderObjects[obj->m_Index-1]->m_Index = obj->m_Index;
        }
    }

    void RenderScene::Update() {
        UpdateUniform();
    }

    void RenderScene::RenderGBuffer(Engine::RCommandBuffer* cmd, Engine::RPipelineLayout* layout) {
        cmd->BindDescriptorSet(layout, m_SceneDescs, 0, Engine::PIPELINE_GRAPHICS);
        for(RenderObject* obj: m_RenderObjects) {
            obj->DrawCall(cmd, layout);
        }
    }
}