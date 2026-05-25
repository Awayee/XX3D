#include "Objects/Public/RenderScene.h"
#include "Core/Public/Concurrency.h"
#include "Objects/Public/RenderCamera.h"
#include "Objects/Public/DirectionalLight.h"
#include "Objects/Public/SkyBox.h"
#include "Window/Public/EngineWindow.h"
#include "Render/public/DefaultResource.h"
#include "Objects/Public/MeshRenderer.h"
#include "Render/Public/GlobalShader.h"

namespace {
    class DeferredLightingVS : public Render::GlobalShader {
        SHADER_PERMUTATION_EMPTY();
        SHADER_BINDING_EMPTY();
        GLOBAL_SHADER_IMPLEMENT(DeferredLightingVS, "DeferredLightingPBR.hlsl", "MainVS", EShaderStageFlags::Vertex);
    };
    class DeferredLightingPS: public Render::GlobalShader {
        SHADER_PERMUTATION_EMPTY();
        BEGIN_SHADER_BINDING
    	SHADER_BINDING(0, UniformBuffer, uCamera, 1)
        SHADER_BINDING(1, UniformBuffer, uLight, 1)
        SHADER_BINDING(2, Texture, inShadowMap, 1)
        SHADER_BINDING(3, Texture, inGBuffer0, 1)
        SHADER_BINDING(4, Texture, inGBuffer1, 1)
        SHADER_BINDING(5, Texture, inDepth, 1)
        SHADER_BINDING(6, Sampler, inPointSampler, 1)
        SHADER_BINDING(7, Sampler, inLinearSampler,1)
        END_SHADER_BINDING
        GLOBAL_SHADER_IMPLEMENT(DeferredLightingPS, "DeferredLightingPBR.hlsl", "MainPS", EShaderStageFlags::Pixel);
    };

    void InitializeDeferredLightingPipelineDesc(RHIGraphicsPipelineStateDesc& desc) {
        RHI* r = RHI::Instance();
        Render::GlobalShaderMap* globalShaderMap = Render::GlobalShaderMap::Instance();
        ERHIFormat colorFormat = r->GetViewport()->GetBackBufferFormat();
        desc.VertexShader = globalShaderMap->GetShader<DeferredLightingVS>()->GetRHI();
        desc.PixelShader = globalShaderMap->GetShader<DeferredLightingPS>()->GetRHI();
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
}

namespace Object {
    TArray<XFunc<void, RenderScene*>>& GetConstructFuncs() {
        static TArray<XFunc<void, RenderScene*>> s_ConstructFuncs {};
        return s_ConstructFuncs;
    }

    TUniquePtr<RenderScene> RenderScene::s_Default;

    void RenderScene::AddConstructFunc(XFunc<void,RenderScene*>&& f) {
        GetConstructFuncs().PushBack(MoveTemp(f));
    }

    RenderScene::RenderScene(){
        // register systems
        for(auto& func: GetConstructFuncs()) {
            func(this);
        }
        // get pso
        RHIGraphicsPipelineStateDesc desc;
        InitializeDeferredLightingPipelineDesc(desc);
        m_DeferredLightingPSO = RHI::Instance()->CreateGraphicsPipelineState(desc);
    	// create camera and lights
        m_DirectionalLight.Reset(new DirectionalLight);
        m_Camera.Reset(new RenderCamera());
        m_Camera->SetView({ { 0, 4, -4 }, { 0, 2, 0 }, { 0, 1, 0 } });
        // primitive renderer
        m_PrimitiveMgr.Reset(new PrimitiveMgr());
    }

    RenderScene::~RenderScene() {
    }

    void RenderScene::Update() {
        // update hzb
        m_HzbBuilder.Update(m_TargetSize.w, m_TargetSize.h, m_Camera->GetViewProjectMatrix());
        // clear draw calls
        m_LightingPassDrawCallQueue.Reset();
        // create deferred lighting draw call firstly
        CreateDeferredLightingDrawCall();

        // update ecs systems for collecting primitives
        SystemUpdate();

        // camera and light
        m_Camera->GetRenderContext().Reset(&m_HzbBuilder);
        m_Camera->UpdateBuffer();
        m_DirectionalLight->Update(m_Camera);

        m_PrimitiveMgr->PreDrawCall();

        // create draw call for base pass and shadow pass
        m_PrimitiveMgr->GenerateBasePassDrawCall(m_Camera.Get());
        for (uint32 i = 0; i < m_DirectionalLight->GetCascadeNum(); ++i) {
            Object::ShadowCamera* shadowCamera = m_DirectionalLight->GetShadowCamera(i);
            shadowCamera->GetRenderContext().Reset(&m_HzbBuilder);
            if (m_DirectionalLight->GetEnableShadow()) {
                m_PrimitiveMgr->GenerateDirectionalShadowDrawCall(
                    shadowCamera,
                    m_DirectionalLight->GetCSMRenderingPSO(),
                    m_DirectionalLight->GetCSMInstancedRenderingPSO());
            }
        }

        m_PrimitiveMgr->Clean();
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
        auto& renderContext = m_Camera->GetRenderContext();
        // gBuffer node
        const Rect renderArea = { 0, 0, m_TargetSize.w, m_TargetSize.h };
        Render::RGRenderNode* basePassNode = rg.CreateRenderNode("BasePass");
        basePassNode->SetRenderArea(renderArea);
        basePassNode->WriteColorTarget(normalNode, 0);
        basePassNode->WriteColorTarget(albedoNode, 1);
        basePassNode->WriteDepthTarget(depthNode);
        // GPU culling
        if(!renderContext.CullingQueue.IsEmpty()) {
            Render::RGBufferNode* cullingResultBuffer = rg.CreateBufferNode(renderContext.CullResultBuffer, "CullResult");
            Render::RGComputeNode* cullingNode = rg.CreateComputeNode("BasePassCulling");
            cullingNode->WriteUAV(cullingResultBuffer);
            basePassNode->ReadSRV(cullingResultBuffer);
            RHITexture* hzb = renderContext.HZB ? renderContext.HZB->GetTexture() : nullptr;
            if(hzb) {
                Render::RGTextureNode* hzbNode = rg.CreateTextureNode(hzb, "HZB");
                cullingNode->ReadSRV(hzbNode);
            }
            cullingNode->SetTask([&cullingQueue=renderContext.CullingQueue](RHICommandBuffer* cmd) {
            	cullingQueue.Execute(cmd);
            });
        }
        basePassNode->SetTask([this](RHICommandBuffer* cmd) {
            m_Camera->GetRenderContext().RenderingQueue.Execute(cmd);
        });

        // copy depth texture
        Render::RGTransferNode* depthCpyNode = rg.CreateTransferNode("CopyDepthToSRV");
        depthCpyNode->CopyTexture(depthNode, depthSRVNode);

        // build hzb
        if(Render::RGTextureNode* hzbNode =m_HzbBuilder.BuildFurthest(m_DepthSRV.Get(), depthSRVNode, rg)) {
            rg.CreateOutputNode(hzbNode, "HZBOutput");
        }

        // directional shadow
        RHITexture* directionalShadowMap = m_DirectionalLight->GetShadowMap();
        Render::RGTextureNode* shadowMapNode = rg.CreateTextureNode(directionalShadowMap, "DirectionalShadowMap");
        const auto& dsmDesc = directionalShadowMap->GetDesc();
        const Rect shadowArea = { 0, 0, dsmDesc.Width, dsmDesc.Height };
        for (uint32 i = 0; i < m_DirectionalLight->GetCascadeNum(); ++i) {
            RenderContext& shadowRenderContext = m_DirectionalLight->GetShadowCamera(i)->GetRenderContext();
            Render::RGRenderNode* csmNode = rg.CreateRenderNode(StringFormat("CSM%u", i));
            // gpu culling
            if (!shadowRenderContext.CullingQueue.IsEmpty()) {
                Render::RGBufferNode* cullingResultBuffer = rg.CreateBufferNode(shadowRenderContext.CullResultBuffer, StringFormat("CullResultCSM%u", i));
                Render::RGComputeNode* cullingNode = rg.CreateComputeNode(StringFormat("CSM%uCulling", i));
                cullingNode->WriteUAV(cullingResultBuffer);
                csmNode->ReadSRV(cullingResultBuffer);
                RHITexture* hzb = shadowRenderContext.HZB ? shadowRenderContext.HZB->GetTexture() : nullptr;
            	if (hzb) {
                    Render::RGTextureNode* hzbNode = rg.CreateTextureNode(hzb, StringFormat("HZBCSM%u", i));
                    cullingNode->ReadSRV(hzbNode);
                }
                cullingNode->SetTask([&cullingQueue = shadowRenderContext.CullingQueue](RHICommandBuffer* cmd) {
                    cullingQueue.Execute(cmd);
                });
            }
            csmNode->SetRenderArea(shadowArea);
            csmNode->WriteDepthTarget(shadowMapNode, dsmDesc.GetSubRes2D((uint16)i, ETextureViewFlags::DepthStencil));
            csmNode->SetTask([this, &shadowRenderContext](RHICommandBuffer* cmd) {
                shadowRenderContext.RenderingQueue.Execute(cmd);
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
            cmd->SetShaderParam(DeferredLightingPS::uCamera, RHIShaderParam::UniformBuffer(m_Camera->GetBuffer()));
            cmd->SetShaderParam(DeferredLightingPS::uLight, RHIShaderParam::UniformBuffer(m_DirectionalLight->GetLightingUniform()));
            RHITexture* directionalShadowMap = m_DirectionalLight->GetShadowMap();
            const RHITextureSubRes directionalShadowMapSubRes = directionalShadowMap->GetDesc().GetSubRes2DArray(ETextureViewFlags::Depth);
            cmd->SetShaderParam(DeferredLightingPS::inShadowMap, RHIShaderParam::Texture(directionalShadowMap, directionalShadowMapSubRes));
            cmd->SetShaderParam(DeferredLightingPS::inGBuffer0, RHIShaderParam::Texture(m_GBufferNormal));
            cmd->SetShaderParam(DeferredLightingPS::inGBuffer1, RHIShaderParam::Texture(m_GBufferAlbedo));
            const RHITextureSubRes depthSrvSubRes = m_Depth->GetDesc().GetSubRes2D(0, ETextureViewFlags::Depth);
            cmd->SetShaderParam(DeferredLightingPS::inDepth, RHIShaderParam::Texture(m_DepthSRV, depthSrvSubRes));
            cmd->SetShaderParam(DeferredLightingPS::inPointSampler, RHIShaderParam::Sampler(Render::DefaultResources::Instance()->GetDefaultSampler(ESamplerFilter::Point, ESamplerAddressMode::Clamp)));
            cmd->SetShaderParam(DeferredLightingPS::inLinearSampler, RHIShaderParam::Sampler(Render::DefaultResources::Instance()->GetDefaultSampler(ESamplerFilter::Bilinear, ESamplerAddressMode::Clamp)));
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
