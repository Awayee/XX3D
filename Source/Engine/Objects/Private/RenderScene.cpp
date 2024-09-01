#include "Core/Public/Concurrency.h"
#include "Objects/Public/RenderScene.h"
#include "Objects/Public/Camera.h"
#include "Objects/Public/DirectionalLight.h"
#include "Objects/Public/SkyBox.h"
#include "Window/Public/EngineWindow.h"
#include "Render/public/DefaultResource.h"
#include "Render/Public/GlobalShader.h"
namespace {
    IMPLEMENT_GLOBAL_SHADER(GBufferVS, "GBuffer.hlsl", "MainVS", SHADER_STAGE_VERTEX_BIT);
    IMPLEMENT_GLOBAL_SHADER(GBufferPS, "GBuffer.hlsl", "MainPS", SHADER_STAGE_PIXEL_BIT);

    IMPLEMENT_GLOBAL_SHADER(DeferredLightingVS, "DeferredLightingPBR.hlsl", "MainVS", SHADER_STAGE_VERTEX_BIT);
    IMPLEMENT_GLOBAL_SHADER(DeferredLightingPS, "DeferredLightingPBR.hlsl", "MainPS", SHADER_STAGE_PIXEL_BIT);

    IMPLEMENT_GLOBAL_SHADER(SkyBoxVS, "SkyBox.hlsl", "MainVS", SHADER_STAGE_VERTEX_BIT);
    IMPLEMENT_GLOBAL_SHADER(SkyBoxPS, "SkyBox.hlsl", "MainPS", SHADER_STAGE_PIXEL_BIT);
}

namespace Object {

    static constexpr ERHIFormat s_NormalFormat{ ERHIFormat::R8G8B8A8_UNORM };
    static constexpr ERHIFormat s_AlbedoFormat{ ERHIFormat::R8G8B8A8_UNORM };

    TUniquePtr<RenderScene> RenderScene::s_Default;
    TArray<Func<void(RenderScene*)>> RenderScene::s_RegisterSystems;

    struct LightUBO {
        Math::FVector3 LightDir; float _padding;
        Math::FVector3 LightColor; float _padding1;
    };

    RenderScene::RenderScene(){
        // register systems
        for(auto& func: s_RegisterSystems) {
            func(this);
        }
        // create camera and lights
        CreateResources();
        // create PSOs
        CreatePSOs();
    }

    RenderScene::~RenderScene() {
    }

    void RenderScene::Update() {
        // draw call
        m_Camera->UpdateBuffer();
        m_DirectionalLight->UpdateShadowCameras(m_Camera);
        m_DirectionalLight->UpdateShadowMapDrawCalls();
        m_DrawCallContext.Reset();
        UpdateSceneDrawCall();
        SystemUpdate();
    }

    void RenderScene::Render(Render::RenderGraph& rg, Render::RGTextureNode* targetNode) {
        const RHITextureDesc& desc = targetNode->GetDesc();
        // check if size or format changed
        USize2D targetSize = { desc.Width, desc.Height };
        if(m_TargetSize != targetSize) {
            m_TargetSize = targetSize;
            CreateTextureResources();
        }

        // ========= create resource nodes ==========
        Render::RGTextureNode* normalNode = rg.CreateTextureNode(m_GBufferNormal, "GBufferNormal");
        Render::RGTextureNode* albedoNode = rg.CreateTextureNode(m_GBufferAlbedo, "GBufferAlbedo");
        Render::RGTextureNode* depthNode = rg.CreateTextureNode(m_Depth, "DepthBuffer");
        Render::RGTextureNode* depthSRVNode = rg.CreateTextureNode(m_DepthSRV, "DepthSRV");

        // ========= create pass nodes ==============
        // TODO parallel run directionalShadow and gBuffer
        // gBuffer node
        const Rect renderArea = { 0, 0, m_TargetSize.w, m_TargetSize.h };
        Render::RGRenderNode* gBufferNode = rg.CreateRenderNode("GBufferPass");
        gBufferNode->SetRenderArea(renderArea);
        gBufferNode->WriteColorTarget(normalNode, 0);
        gBufferNode->WriteColorTarget(albedoNode, 1);
        gBufferNode->WriteDepthTarget(depthNode);
        gBufferNode->SetTask([this](RHICommandBuffer* cmd) {
            m_DrawCallContext.ExecuteDraCall(Render::EDrawCallQueueType::BasePass, cmd);
        });

        // copy depth texture
        Render::RGTransferNode* depthCpyNode = rg.CreateTransferNode("CopyDepthToSRV");
        depthCpyNode->CopyTexture(depthNode, depthSRVNode, {});

        // directional shadow
        RHITexture* directionalShadowMap = m_DirectionalLight->GetShadowMap();
        Render::RGTextureNode* shadowMapNode = rg.CreateTextureNode(directionalShadowMap, "DirectionalShadowMap");
        const Rect shadowArea = { 0, 0, directionalShadowMap->GetDesc().Width, directionalShadowMap->GetDesc().Height };
        for (uint32 i = 0; i < m_DirectionalLight->GetCascadeNum(); ++i) {
            Render::RGRenderNode* csmNode = rg.CreateRenderNode(StringFormat("CSM%u", i));
            csmNode->SetRenderArea(shadowArea);
            csmNode->WriteDepthTarget(shadowMapNode, { 0, 1, (uint8)i, 1 });
            csmNode->SetTask([this, i](RHICommandBuffer* cmd) {
                m_DirectionalLight->GetDrawCallQueue(i).Execute(cmd);
            });
        }

        // deferred lighting
        Render::RGRenderNode* deferredLightingNode = rg.CreateRenderNode("DeferredLightingPass");
        deferredLightingNode->ReadSRV(shadowMapNode);
        deferredLightingNode->ReadSRV(normalNode);
        deferredLightingNode->ReadSRV(albedoNode);
        deferredLightingNode->ReadSRV(depthSRVNode);
        deferredLightingNode->WriteColorTarget(targetNode, 0);
        deferredLightingNode->ReadDepthTarget(depthNode);
        deferredLightingNode->SetRenderArea(renderArea);
        deferredLightingNode->SetTask([this](RHICommandBuffer* cmd) {
            m_DrawCallContext.ExecuteDraCall(Render::EDrawCallQueueType::DeferredLighting, cmd);
        });
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
        m_DrawCallContext.PushDrawCall(Render::EDrawCallQueueType::BasePass, [this](RHICommandBuffer* cmd) {
            cmd->BindGraphicsPipeline(m_GBufferPSO);
            cmd->SetShaderParam(0, 0, RHIShaderParam::UniformBuffer(m_Camera->GetBuffer()));
        });

        // deferred lighting
        m_DrawCallContext.PushDrawCall(Render::EDrawCallQueueType::DeferredLighting, [this](RHICommandBuffer* cmd) {
            cmd->BindGraphicsPipeline(m_DeferredLightingPSO);
            cmd->SetShaderParam(0, 0, RHIShaderParam::UniformBuffer(m_Camera->GetBuffer()));
            cmd->SetShaderParam(0, 1, RHIShaderParam::UniformBuffer(m_DirectionalLight->GetUniform()));
            cmd->SetShaderParam(0, 2, RHIShaderParam::Texture(m_DirectionalLight->GetShadowMap(), ETextureSRVType::Depth));
            cmd->SetShaderParam(0, 3, RHIShaderParam::Texture(m_GBufferNormal));
            cmd->SetShaderParam(0, 4, RHIShaderParam::Texture(m_GBufferAlbedo));
            cmd->SetShaderParam(0, 5, RHIShaderParam::Texture(m_DepthSRV, ETextureSRVType::Depth, {}));
            cmd->SetShaderParam(0, 6, RHIShaderParam::Sampler(Render::DefaultResources::Instance()->GetDefaultSampler(ESamplerFilter::Point, ESamplerAddressMode::Clamp)));
            cmd->SetShaderParam(0, 7, RHIShaderParam::Sampler(Render::DefaultResources::Instance()->GetDefaultSampler(ESamplerFilter::Bilinear, ESamplerAddressMode::Clamp)));
        	cmd->Draw(6, 1, 0, 0);
        });

        // sky box rendering after deferred lighting
        m_SkyBox->CreateDrawCall(m_DrawCallContext.GetDrawCallQueue(Render::EDrawCallQueueType::DeferredLighting));
    }

    void RenderScene::CreateResources() {
        m_DirectionalLight.Reset(new DirectionalLight);
        m_DirectionalLight->SetRotation({ 0.0f, 0.0f, 0.0f });
        m_Camera.Reset(new RenderCamera());
        m_Camera->SetView({{ 0, 4, -4 }, { 0, 2, 0 }, { 0, 1, 0 }});
        m_SkyBox.Reset(new SkyBox(m_Camera));
    }

    void RenderScene::CreateTextureResources() {
        RHI* r = RHI::Instance();
        RHITextureDesc desc = RHITextureDesc::Texture2D();
        desc.Flags = TEXTURE_FLAG_COLOR_TARGET | TEXTURE_FLAG_SRV;
        desc.Width = m_TargetSize.w;
        desc.Height = m_TargetSize.h;
        desc.Format = s_NormalFormat;
        m_GBufferNormal = r->CreateTexture(desc);

        desc.Format = s_AlbedoFormat;
        m_GBufferAlbedo = r->CreateTexture(desc);

        desc.Format = r->GetDepthFormat();
        desc.Flags = TEXTURE_FLAG_DEPTH_TARGET | TEXTURE_FLAG_STENCIL | TEXTURE_FLAG_CPY_SRC;
        m_Depth = r->CreateTexture(desc);
        desc.Flags = TEXTURE_FLAG_DEPTH_TARGET | TEXTURE_FLAG_STENCIL | TEXTURE_FLAG_SRV | TEXTURE_FLAG_CPY_DST;
        m_DepthSRV = r->CreateTexture(desc);
    }

    void RenderScene::CreatePSOs() {
        RHI* r = RHI::Instance();
        Render::GlobalShaderMap* globalShaderMap = Render::GlobalShaderMap::Instance();
        ERHIFormat colorFormat = r->GetViewport()->GetBackBufferFormat();
        // base pass
        {
            const TStaticArray<ERHIFormat, 2> colorFormats = { s_NormalFormat, s_AlbedoFormat };
            const ERHIFormat depthFormat = RHI::Instance()->GetDepthFormat();
            RHIGraphicsPipelineStateDesc desc{};
            desc.VertexShader = globalShaderMap->GetShader<GBufferVS>()->GetRHI();
            desc.PixelShader = globalShaderMap->GetShader<GBufferPS>()->GetRHI();
            // ds layout
            auto& layout = desc.Layout;
            layout.Resize(3);
            layout[0] = {
                {EBindingType::UniformBuffer, SHADER_STAGE_VERTEX_BIT | SHADER_STAGE_PIXEL_BIT},
            };
            layout[1] = {
                {EBindingType::UniformBuffer, SHADER_STAGE_VERTEX_BIT}
            };
            layout[2] = {
                {EBindingType::Texture, SHADER_STAGE_PIXEL_BIT},
                {EBindingType::Sampler, SHADER_STAGE_PIXEL_BIT},
            };
            // vertex input
            auto& vi = desc.VertexInput;
            vi.Bindings = { {0, sizeof(Asset::AssetVertex), false} };
            vi.Attributes = {
                {0, 0, ERHIFormat::R32G32B32_SFLOAT, 0},// position
                {1, 0, ERHIFormat::R32G32B32_SFLOAT, offsetof(Asset::AssetVertex, Normal)},//normal
                {2, 0, ERHIFormat::R32G32B32_SFLOAT, offsetof(Asset::AssetVertex, Tangent)},//tangent
                {3, 0, ERHIFormat::R32G32_SFLOAT, offsetof(Asset::AssetVertex, UV)},// uv
            };

            desc.BlendDesc.BlendStates = { {false}, {false} };
            desc.RasterizerState = { ERasterizerFill::Solid, ERasterizerCull::Back };
            desc.DepthStencilState = { true, ECompareType::Less, false };
            desc.PrimitiveTopology = EPrimitiveTopology::TriangleList;
            desc.ColorFormats.Resize(colorFormats.Size());
            for (uint32 i = 0; i < colorFormats.Size(); ++i) {
                desc.ColorFormats[i] = colorFormats[i];
            }
            desc.DepthStencilFormat = depthFormat;
            desc.NumSamples = 1;
            m_GBufferPSO = r->CreateGraphicsPipelineState(desc);
        }
        // deferred lighting pass
        {
            RHIGraphicsPipelineStateDesc desc{};
            desc.VertexShader = globalShaderMap->GetShader<DeferredLightingVS>()->GetRHI();
            desc.PixelShader = globalShaderMap->GetShader<DeferredLightingPS>()->GetRHI();
            auto& layout = desc.Layout;
            layout.Resize(1);
            layout[0] = {
                {EBindingType::UniformBuffer, SHADER_STAGE_PIXEL_BIT},// camera
                {EBindingType::UniformBuffer, SHADER_STAGE_PIXEL_BIT},// light uniform
                {EBindingType::Texture, SHADER_STAGE_PIXEL_BIT}, // shadow map
                {EBindingType::Texture, SHADER_STAGE_PIXEL_BIT},// normal tex
                {EBindingType::Texture, SHADER_STAGE_PIXEL_BIT},// albedo tex
                {EBindingType::Texture, SHADER_STAGE_PIXEL_BIT},// depth tex
                {EBindingType::Sampler, SHADER_STAGE_PIXEL_BIT},// point sampler
                {EBindingType::Sampler, SHADER_STAGE_PIXEL_BIT},// linear sampler
            };
            desc.VertexInput = {};
            desc.BlendDesc.BlendStates = { {false} };
            desc.RasterizerState = { ERasterizerFill::Solid, ERasterizerCull::Back };
            desc.DepthStencilState = { false, ECompareType::Always, false };
            desc.PrimitiveTopology = EPrimitiveTopology::TriangleList;
            desc.ColorFormats.PushBack(colorFormat);
            desc.DepthStencilFormat = RHI::Instance()->GetDepthFormat();
            desc.NumSamples = 1;
            m_DeferredLightingPSO = r->CreateGraphicsPipelineState(desc);
        }
    }
}
