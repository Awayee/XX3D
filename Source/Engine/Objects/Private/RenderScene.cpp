#include "System/Public/EngineConfig.h"
#include "Core/Public/Concurrency.h"

#include "Objects/Public/RenderScene.h"
#include "Objects/Public/Camera.h"
#include "Objects/Public/Light/DirectionalLight.h"

namespace Object {
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
        m_DirectionalLight->SetDir({-1, -1, -1});
        auto size = RHI::Instance()->GetViewport()->GetSize();
        m_Camera.Reset(new Camera(EProjectiveType::Perspective, (float)size.w / (float)size.h, 0.1f, 1000.0f, Math::Deg2Rad * 75.0f));
        m_Camera->SetView({ 0, 4, -4 }, { 0, 2, 0}, { 0, 1, 0 });
        m_UniformBuffer = RHI::Instance()->CreateBuffer({ EBufferFlagBit::BUFFER_FLAG_UNIFORM, sizeof(LightUBO) + sizeof(CameraUBO), 0 });
    }

    RenderScene* RenderScene::GetDefaultScene() {
        if(nullptr == s_Default.Get()) {
            static Mutex m;
            MutexLock lock(m);
            if (nullptr == s_Default.Get()) {
                s_Default.Reset(new RenderScene);
            }
        }
        return s_Default.Get();
    }

    RenderScene::RenderScene() {
        CreateResources();
    }

    RenderScene::~RenderScene() {
    }

    RenderScene::RenderScene(RenderScene&& rhs)noexcept:
	m_RenderObjects(MoveTemp(rhs.m_RenderObjects)),
	m_DirectionalLight(MoveTemp(rhs.m_DirectionalLight)),
	m_Camera(MoveTemp(rhs.m_Camera)) {
    }

    RenderScene& RenderScene::operator=(RenderScene&& rhs) noexcept {
        m_RenderObjects = MoveTemp(rhs.m_RenderObjects);
        m_DirectionalLight = MoveTemp(rhs.m_DirectionalLight);
        m_Camera = MoveTemp(rhs.m_Camera);
        return *this;
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
        if (!m_RenderObjects.IsEmpty()) {
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

    void RenderScene::GenerateDrawCall(Render::DrawCallQueue& queue) {
    }
}
