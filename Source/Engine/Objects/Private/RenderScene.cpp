#include "Core/Public/Concurrency.h"
#include "Objects/Public/RenderScene.h"
#include "Objects/Public/Camera.h"
#include "Objects/Public/DirectionalLight.h"
#include "Objects/Public/SkyBox.h"
#include "Window/Public/EngineWindow.h"
#include "Render/public/DefaultResource.h"
#include "Render/Public/GlobalShader.h"
namespace {
    IMPLEMENT_GLOBAL_SHADER(GBufferVS, "GBuffer.hlsl", "MainVS", EShaderStageFlags::Vertex);
    IMPLEMENT_GLOBAL_SHADER(GBufferPS, "GBuffer.hlsl", "MainPS", EShaderStageFlags::Pixel);

    IMPLEMENT_GLOBAL_SHADER(DeferredLightingVS, "DeferredLightingPBR.hlsl", "MainVS", EShaderStageFlags::Vertex);
    IMPLEMENT_GLOBAL_SHADER(DeferredLightingPS, "DeferredLightingPBR.hlsl", "MainPS", EShaderStageFlags::Pixel);

    IMPLEMENT_GLOBAL_SHADER(SkyBoxVS, "SkyBox.hlsl", "MainVS", EShaderStageFlags::Vertex);
    IMPLEMENT_GLOBAL_SHADER(SkyBoxPS, "SkyBox.hlsl", "MainPS", EShaderStageFlags::Pixel);
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
        // create PSOs
        CreatePSOs();
        // create camera and lights
        CreateResources();
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
        depthCpyNode->CopyTexture(depthNode, depthSRVNode);

        // directional shadow
        RHITexture* directionalShadowMap = m_DirectionalLight->GetShadowMap();
        Render::RGTextureNode* shadowMapNode = rg.CreateTextureNode(directionalShadowMap, "DirectionalShadowMap");
        const auto& dsmDesc = directionalShadowMap->GetDesc();
        const Rect shadowArea = { 0, 0, dsmDesc.Width, dsmDesc.Height };
        for (uint32 i = 0; i < m_DirectionalLight->GetCascadeNum(); ++i) {
            Render::RGRenderNode* csmNode = rg.CreateRenderNode(StringFormat("CSM%u", i));
            csmNode->SetRenderArea(shadowArea);
            csmNode->WriteDepthTarget(shadowMapNode, dsmDesc.GetSubRes2D((uint16)i, ETextureViewFlags::DepthStencil));
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
            cmd->SetShaderParam(0, 0, RHIShaderParam::UniformBuffer(m_Camera->GertUniformBuffer()));
        });

        // deferred lighting
        m_DrawCallContext.PushDrawCall(Render::EDrawCallQueueType::DeferredLighting, [this](RHICommandBuffer* cmd) {
            cmd->BindGraphicsPipeline(m_DeferredLightingPSO);
            cmd->SetShaderParam(0, 0, RHIShaderParam::UniformBuffer(m_Camera->GertUniformBuffer()));
            cmd->SetShaderParam(0, 1, RHIShaderParam::UniformBuffer(m_DirectionalLight->GetUniform()));
            RHITexture* directionalShadowMap = m_DirectionalLight->GetShadowMap();
            const RHITextureSubRes directionalShadowMapSubRes = directionalShadowMap->GetDesc().GetSubRes2DArray(ETextureViewFlags::Depth);
            cmd->SetShaderParam(0, 2, RHIShaderParam::Texture(directionalShadowMap, directionalShadowMapSubRes));
            cmd->SetShaderParam(0, 3, RHIShaderParam::Texture(m_GBufferNormal));
            cmd->SetShaderParam(0, 4, RHIShaderParam::Texture(m_GBufferAlbedo));
            const RHITextureSubRes depthSrvSubRes = m_Depth->GetDesc().GetSubRes2D(0, ETextureViewFlags::Depth);
            cmd->SetShaderParam(0, 5, RHIShaderParam::Texture(m_DepthSRV, depthSrvSubRes));
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
        desc.Flags = ETextureFlags::ColorTarget | ETextureFlags::SRV;
        desc.Width = m_TargetSize.w;
        desc.Height = m_TargetSize.h;
        desc.Format = s_NormalFormat;
        m_GBufferNormal = r->CreateTexture(desc);

        desc.Format = s_AlbedoFormat;
        m_GBufferAlbedo = r->CreateTexture(desc);

        desc.Format = r->GetDepthFormat();
        desc.Flags = ETextureFlags::DepthStencilTarget | ETextureFlags::CopySrc;
        desc.ClearValue.DepthStencil = { 1.0f, 0u };
        m_Depth = r->CreateTexture(desc);
        desc.Flags = ETextureFlags::DepthStencilTarget | ETextureFlags::SRV | ETextureFlags::CopyDst;
        desc.ClearValue.DepthStencil = { 1.0f, 0u };
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
                {EBindingType::UniformBuffer, EShaderStageFlags::Vertex | EShaderStageFlags::Pixel},
            };
            layout[1] = {
                {EBindingType::UniformBuffer, EShaderStageFlags::Vertex}
            };
            layout[2] = {
                {EBindingType::Texture, EShaderStageFlags::Pixel},
                {EBindingType::Sampler, EShaderStageFlags::Pixel},
            };
            // vertex input
            auto& vi = desc.VertexInput;
            vi.Bindings = { {0, sizeof(Asset::AssetVertex), false} };
            vi.Attributes = {
                {POSITION, 0, 0, 0, ERHIFormat::R32G32B32_SFLOAT, 0},// position
                {NORMAL, 0, 1, 0, ERHIFormat::R32G32B32_SFLOAT, offsetof(Asset::AssetVertex, Normal)},//normal
                {TANGENT, 0, 2, 0, ERHIFormat::R32G32B32_SFLOAT, offsetof(Asset::AssetVertex, Tangent)},//tangent
                {TEXCOORD, 0, 3, 0, ERHIFormat::R32G32_SFLOAT, offsetof(Asset::AssetVertex, UV)},// uv
            };

            desc.BlendDesc.BlendStates = { {false}, {false} };
            desc.RasterizerState = { ERasterizerFill::Solid, ERasterizerCull::Back };
            desc.DepthStencilState = { true, true, ECompareType::Less, false };
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
                {EBindingType::UniformBuffer, EShaderStageFlags::Pixel},// camera
                {EBindingType::UniformBuffer, EShaderStageFlags::Pixel},// light uniform
                {EBindingType::Texture, EShaderStageFlags::Pixel}, // shadow map
                {EBindingType::Texture, EShaderStageFlags::Pixel},// normal tex
                {EBindingType::Texture, EShaderStageFlags::Pixel},// albedo tex
                {EBindingType::Texture, EShaderStageFlags::Pixel},// depth tex
                {EBindingType::Sampler, EShaderStageFlags::Pixel},// point sampler
                {EBindingType::Sampler, EShaderStageFlags::Pixel},// linear sampler
            };
            desc.VertexInput = {};
            desc.BlendDesc.BlendStates = { {false} };
            desc.RasterizerState = { ERasterizerFill::Solid, ERasterizerCull::Back };
            desc.DepthStencilState = { false, false, ECompareType::Always, false };
            desc.PrimitiveTopology = EPrimitiveTopology::TriangleList;
            desc.ColorFormats.PushBack(colorFormat);
            desc.DepthStencilFormat = RHI::Instance()->GetDepthFormat();
            desc.NumSamples = 1;
            m_DeferredLightingPSO = r->CreateGraphicsPipelineState(desc);
        }
    }
}
