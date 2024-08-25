#include "Core/Public/Concurrency.h"
#include "Objects/Public/RenderScene.h"
#include "Objects/Public/Camera.h"
#include "Objects/Public/DirectionalLight.h"
#include "Window/Public/EngineWindow.h"
#include "Render/public/DefaultResource.h"
#include "Render/Public/MeshRender.h"

namespace Object {

    static constexpr ERHIFormat s_NormalFormat{ ERHIFormat::B8G8R8A8_UNORM };
    static constexpr ERHIFormat s_AlbedoFormat{ ERHIFormat::B8G8R8A8_UNORM };

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

    RenderScene::RenderScene(): m_RenderTarget(nullptr), m_ViewportDirty(false), m_RenderTargetFormatDirty(false) {
        // register systems
        for(auto& func: s_RegisterSystems) {
            func(this);
        }
        // create pso
        const TStaticArray<ERHIFormat, 2> formats = { s_NormalFormat, s_AlbedoFormat };
        const ERHIFormat depthFormat = RHI::Instance()->GetDepthFormat();
        MeshGBufferPSO = Render::CreateMeshGBufferRenderPSO(formats, depthFormat);
        // create camera and lights
        CreateResources();
        const auto windowSize = Engine::EngineWindow::Instance()->GetWindowSize();
        SetViewportSize(windowSize);
        Render::ViewportRenderer::Instance()->SetRenderScene(this);
    }

    RenderScene::~RenderScene() {
    }

    void RenderScene::Update() {
        // draw call
        m_DirectionalLight->UpdateShadowCameras(m_Camera);
        m_DirectionalLight->UpdateShadowMapDrawCalls();
        m_DrawCallContext.Reset();
        UpdateSceneDrawCall();
        SystemUpdate();
    }

    void RenderScene::SetViewportSize(const USize2D& size) {
        m_Camera->SetAspect((float)size.w / (float)size.h);
        m_ViewportSize = { size.w, size.h };
        m_ViewportDirty = true;
    }

    void RenderScene::SetRenderTarget(RHITexture* target, RHITextureSubDesc sub) {
        // check if the format of render target changed
        auto getFormat = [](RHITexture* tex) {return tex ? tex->GetDesc().Format : ERHIFormat::Undefined; };
        m_RenderTargetFormatDirty = getFormat(m_RenderTarget) != getFormat(target);
        m_RenderTarget = target;
        m_RenderTargetSub = sub;
    }

    Render::RGTextureNode* RenderScene::Render(Render::RenderGraph& rg) {
        if(!m_RenderTarget) {
            return nullptr;
        }
        if(m_ViewportDirty || m_RenderTargetFormatDirty) {
            Render::ViewportRenderer::Instance()->WaitAllFence();
            if(m_ViewportDirty) {
                CreateRTTextures();
                m_ViewportDirty = false;
            }
            if (m_RenderTargetFormatDirty) {
                m_DeferredLightingPSO = Render::CreateDeferredLightingPSO(m_RenderTarget->GetDesc().Format);
                m_RenderTargetFormatDirty = false;
            }
        }
        // ========= create resource nodes ==========
        Render::RGTextureNode* normalNode = rg.CreateTextureNode(m_GBufferNormal, "GBufferNormal");
        Render::RGTextureNode* albedoNode = rg.CreateTextureNode(m_GBufferAlbedo, "GBufferAlbedo");
        Render::RGTextureNode* depthNode = rg.CreateTextureNode(m_Depth, "DepthBuffer");
        Render::RGTextureNode* targetNode = rg.CreateTextureNode(m_RenderTarget, "SceneRenderTarget");

        // ========= create pass nodes ==============
        // TODO parallel run directionalShadow and gBuffer
        // gBuffer node
        const Rect renderArea = { 0, 0, m_ViewportSize.w, m_ViewportSize.h };
        Render::RGRenderNode* gBufferNode = rg.CreateRenderNode("GBufferPass");
        gBufferNode->SetArea(renderArea);
        gBufferNode->WriteColorTarget(normalNode, 0);
        gBufferNode->WriteColorTarget(albedoNode, 1);
        gBufferNode->WriteDepthTarget(depthNode);
        gBufferNode->SetTask([this](RHICommandBuffer* cmd) {
            cmd->BindGraphicsPipeline(MeshGBufferPSO);
            m_DrawCallContext.ExecuteDraCall(Render::EDrawCallQueueType::BasePass, cmd);
        });

        // directional shadow
        RHITexture* directionalShadowMap = m_DirectionalLight->GetShadowMap();
        Render::RGTextureNode* shadowMapNode = rg.CreateTextureNode(directionalShadowMap, "DirectionalShadowMap");
        if(m_DirectionalLight->GetEnableShadow()){
            const Rect shadowArea = { 0, 0, directionalShadowMap->GetDesc().Width, directionalShadowMap->GetDesc().Height };
            for (uint32 i = 0; i < m_DirectionalLight->GetCascadeNum(); ++i) {
                Render::RGRenderNode* csmNode = rg.CreateRenderNode(StringFormat("CSM%u", i));
                csmNode->SetArea(shadowArea);
                csmNode->WriteDepthTarget(shadowMapNode, { 0, 1, (uint8)i, 1 });
                csmNode->SetTask([this, i](RHICommandBuffer* cmd) {
                    m_DirectionalLight->GetDrawCallQueue(i).Execute(cmd);
                });
            }
        }

        // deferred lighting
        Render::RGRenderNode* deferredLightingNode = rg.CreateRenderNode("DeferredLightingPass");
        deferredLightingNode->ReadSRV(shadowMapNode);
        deferredLightingNode->ReadSRV(normalNode);
        deferredLightingNode->ReadSRV(albedoNode);
        deferredLightingNode->ReadSRV(depthNode);
        deferredLightingNode->WriteColorTarget(targetNode, 0);
        deferredLightingNode->SetArea(renderArea);
        deferredLightingNode->SetTask([this](RHICommandBuffer* cmd) {
            cmd->BindGraphicsPipeline(m_DeferredLightingPSO.Get());
            m_DrawCallContext.ExecuteDraCall(Render::EDrawCallQueueType::DeferredLighting, cmd);
        });
        return targetNode;
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

    void RenderScene::UpdateSceneDrawCall() {
		CameraUBO cameraUBO;
		cameraUBO.View = m_Camera->GetViewMatrix();
		cameraUBO.Proj = m_Camera->GetProjectMatrix();
		cameraUBO.VP = m_Camera->GetViewProjectMatrix();
		cameraUBO.InvVP = m_Camera->GetInvViewProjectMatrix();
		cameraUBO.CamPos = m_Camera->GetView().Eye;
        m_CameraUniform->UpdateData(&cameraUBO, sizeof(SceneData), 0);

        m_DrawCallContext.PushDrawCall(Render::EDrawCallQueueType::BasePass, [this](RHICommandBuffer* cmd) {
            cmd->SetShaderParam(0, 0, RHIShaderParam::UniformBuffer(m_CameraUniform));
        });
        m_DrawCallContext.PushDrawCall(Render::EDrawCallQueueType::DeferredLighting, [this](RHICommandBuffer* cmd) {
            cmd->SetShaderParam(0, 0, RHIShaderParam::UniformBuffer(m_CameraUniform));
            cmd->SetShaderParam(0, 1, RHIShaderParam::UniformBuffer(m_DirectionalLight->GetUniform()));
            cmd->SetShaderParam(0, 2, RHIShaderParam::Texture(m_DirectionalLight->GetShadowMap(), ETextureSRVType::Depth));
            cmd->SetShaderParam(0, 3, RHIShaderParam::Texture(m_GBufferNormal));
            cmd->SetShaderParam(0, 4, RHIShaderParam::Texture(m_GBufferAlbedo));
            cmd->SetShaderParam(0, 5, RHIShaderParam::Texture(m_Depth, ETextureSRVType::Depth, {}));
            cmd->SetShaderParam(0, 6, RHIShaderParam::Sampler(Render::DefaultResources::Instance()->GetDefaultSampler(ESamplerFilter::Point, ESamplerAddressMode::Clamp)));
            cmd->SetShaderParam(0, 7, RHIShaderParam::Sampler(Render::DefaultResources::Instance()->GetDefaultSampler(ESamplerFilter::Bilinear, ESamplerAddressMode::Clamp)));
        	cmd->Draw(6, 1, 0, 0);
        });
    }

    void RenderScene::CreateResources() {
        m_DirectionalLight.Reset(new DirectionalLight);
        m_DirectionalLight->SetRotation({ 0.0f, 0.0f, 0.0f });
        m_Camera.Reset(new RenderCamera());
        m_Camera->SetView({{ 0, 4, -4 }, { 0, 2, 0 }, { 0, 1, 0 }});
        m_CameraUniform = RHI::Instance()->CreateBuffer({ EBufferFlagBit::BUFFER_FLAG_UNIFORM, sizeof(CameraUBO), 0 });
    }

    void RenderScene::CreateRTTextures() {
        RHI* r = RHI::Instance();
        RHITextureDesc desc = RHITextureDesc::Texture2D();
        desc.Flags = TEXTURE_FLAG_COLOR_TARGET | TEXTURE_FLAG_SRV;
        desc.Width = m_ViewportSize.w;
        desc.Height = m_ViewportSize.h;
        desc.Format = s_NormalFormat;
        m_GBufferNormal = r->CreateTexture(desc);

        desc.Format = s_AlbedoFormat;
        m_GBufferAlbedo = r->CreateTexture(desc);

        desc.Format = r->GetDepthFormat();
        desc.Flags = TEXTURE_FLAG_DEPTH_TARGET | TEXTURE_FLAG_STENCIL | TEXTURE_FLAG_SRV;
        m_Depth = r->CreateTexture(desc);
    }
}
