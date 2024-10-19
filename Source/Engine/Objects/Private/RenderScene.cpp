#include "Core/Public/Concurrency.h"
#include "Objects/Public/RenderScene.h"
#include "Objects/Public/Camera.h"
#include "Objects/Public/DirectionalLight.h"
#include "Objects/Public/SkyBox.h"
#include "Window/Public/EngineWindow.h"
#include "Render/public/DefaultResource.h"
#include "Objects/Public/RenderResource.h"
#include "Render/Public/GlobalShader.h"
namespace {
    class DeferredLightingVS : public Render::GlobalShader {
        GLOBAL_SHADER_IMPLEMENT(DeferredLightingVS, "DeferredLightingPBR.hlsl", "MainVS", EShaderStageFlags::Vertex);
        SHADER_PERMUTATION_EMPTY();
    };
    class DeferredLightingPS: public Render::GlobalShader {
        GLOBAL_SHADER_IMPLEMENT(DeferredLightingPS, "DeferredLightingPBR.hlsl", "MainPS", EShaderStageFlags::Pixel);
        SHADER_PERMUTATION_EMPTY();
    };

    void InitializeDeferredLightingPipelineDesc(RHIGraphicsPipelineStateDesc& desc) {
        RHI* r = RHI::Instance();
        Render::GlobalShaderMap* globalShaderMap = Render::GlobalShaderMap::Instance();
        ERHIFormat colorFormat = r->GetViewport()->GetBackBufferFormat();
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
        desc.ColorFormats[0] = colorFormat;
        desc.NumColorTargets = 1;
        desc.DepthStencilFormat = RHI::Instance()->GetDepthFormat();
        desc.NumSamples = 1;
    }
    static const uint32 s_DeferredLightingPSOID{ Object::StaticResourceMgr::RegisterPSOInitializer(InitializeDeferredLightingPipelineDesc) };
}

namespace Object {
    TArray<Func<void(RenderScene*)>>& GetConstructFuncs() {
        static TArray<Func<void(RenderScene*)>> s_ConstructFuncs {};
        return s_ConstructFuncs;
    }

    TUniquePtr<RenderScene> RenderScene::s_Default;

    void RenderScene::AddConstructFunc(Func<void(RenderScene*)>&& f) {
        GetConstructFuncs().PushBack(MoveTemp(f));
    }

    RenderScene::RenderScene(){
        // register systems
        for(auto& func: GetConstructFuncs()) {
            func(this);
        }
        // get pso
        m_DeferredLightingPSO = StaticResourceMgr::Instance()->GetGraphicsPipelineState(s_DeferredLightingPSOID);
    	// create camera and lights
        m_DirectionalLight.Reset(new DirectionalLight);
        m_Camera.Reset(new RenderCamera());
        m_Camera->SetView({ { 0, 4, -4 }, { 0, 2, 0 }, { 0, 1, 0 } });
    }

    RenderScene::~RenderScene() {
    }

    void RenderScene::Update() {
        // draw call
        m_Camera->UpdateBuffer();
        m_DirectionalLight->Update(m_Camera);
        m_BasePassDrawCallQueue.Reset();
        m_LightingPassDrawCallQueue.Reset();
        CreateDeferredLightingDrawCall();
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
            GetBasePasDrawCallQueue().Execute(cmd);
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
            GetLightingPassDrawCallQueue().Execute(cmd);
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

    void RenderScene::CreateDeferredLightingDrawCall() {
        // deferred lighting
        GetLightingPassDrawCallQueue().PushDrawCall([this](RHICommandBuffer* cmd) {
            cmd->BindGraphicsPipeline(m_DeferredLightingPSO);
            cmd->SetShaderParam(0, 0, RHIShaderParam::UniformBuffer(m_Camera->GetUniformBuffer()));
            cmd->SetShaderParam(0, 1, RHIShaderParam::UniformBuffer(m_DirectionalLight->GetLightingUniform()));
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
    }

    void RenderScene::CreateTextureResources() {
        RHI* r = RHI::Instance();
        RHITextureDesc desc = RHITextureDesc::Texture2D();
        desc.Flags = ETextureFlags::ColorTarget | ETextureFlags::SRV;
        desc.Width = m_TargetSize.w;
        desc.Height = m_TargetSize.h;
        desc.Format = GetGBufferNormalFormat();
        m_GBufferNormal = r->CreateTexture(desc);

        desc.Format = GetGBufferAlbedoFormat();
        m_GBufferAlbedo = r->CreateTexture(desc);

        desc.Format = r->GetDepthFormat();
        desc.Flags = ETextureFlags::DepthStencilTarget | ETextureFlags::CopySrc;
        desc.ClearValue.DepthStencil = { 1.0f, 0u };
        m_Depth = r->CreateTexture(desc);
        desc.Flags = ETextureFlags::DepthStencilTarget | ETextureFlags::SRV | ETextureFlags::CopyDst;
        desc.ClearValue.DepthStencil = { 1.0f, 0u };
        m_DepthSRV = r->CreateTexture(desc);
    }
}
