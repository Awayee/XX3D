#include "Render/Public/Renderer.h"
#include "Render/Public/MeshRender.h"
#include "Render/Public/DefaultResource.h"
#include "System/Public/FrameCounter.h"
#include "RHI/Public/ImGuiRHI.h"
#include "Window/Public/EngineWindow.h"

namespace Render {

	ERHIFormat GetPresentFormat() {
		return RHI::Instance()->GetViewport()->GetCurrentBackBuffer()->GetDesc().Format;
	}

	CmdPool::CmdPool(): m_AllocatedIndex(0) {}

	RHICommandBuffer* CmdPool::GetCmd() {
		if(m_AllocatedIndex >= m_Cmds.Size()) {
			for(uint32 i=m_Cmds.Size(); i<= m_AllocatedIndex; ++i) {
				m_Cmds.PushBack(RHI::Instance()->CreateCommandBuffer());
			}
		}
		return m_Cmds[m_AllocatedIndex++].Get();

	}

	void CmdPool::Reset() {
		m_AllocatedIndex = 0;
	}

	void CmdPool::GC() {
		if(!m_Cmds.IsEmpty()) {
			for (uint32 i = m_Cmds.Size() - 1; i > m_AllocatedIndex; --i) {
				m_Cmds.PopBack();
			}
		}
	}

	ViewportRenderer::ViewportRenderer(): m_SizeDirty(false) {
		// Create fences
		for(uint32 i=0; i<RHI_FRAME_IN_FLIGHT_MAX; ++i) {
			m_Fences[i] = RHI::Instance()->CreateFence(true);
		}
		const TStaticArray<ERHIFormat, 2> formats = { s_NormalFormat, s_AlbedoFormat };
		MeshGBufferPSO = CreateMeshGBufferRenderPSO(formats, RHI::Instance()->GetDepthFormat());
		m_DeferredLightingPSO = CreateDeferredLightingPSO(GetPresentFormat());

		// init window area
		m_CacheWindowSize = Engine::EngineWindow::Instance()->GetWindowSize();
		Engine::EngineWindow::Instance()->RegisterOnWindowSizeFunc([this](uint32 w, uint32 h) {OnWindowSizeChanged(w, h); });
		CreateTextures();
	}

	ViewportRenderer::~ViewportRenderer() {
		WaitAllFence();
	}

	void ViewportRenderer::Execute(DrawCallContext* drawCallContext) {
		if(!SizeValid()) {
			return;
		}
		const uint32 frameIndex = FrameCounter::GetFrame() % RHI_FRAME_IN_FLIGHT_MAX;
		RHIFence* fence = m_Fences[frameIndex].Get();

		// Wait fence before rendering.
		fence->Wait();

		if (m_SizeDirty) {
			WaitAllFence();
			RHI::Instance()->GetViewport()->SetSize(m_CacheWindowSize);
			CreateTextures();
			m_SizeDirty = false;
		}

		RHIViewport* viewport = RHI::Instance()->GetViewport();
		RHITexture* backBuffer = viewport->AcquireBackBuffer();
		if (!backBuffer) {
			return;
		}

		RenderGraph rg;
		// ========= create resource nodes ==========
		RGTextureNode* normalNode = rg.CreateTextureNode(m_GBufferNormal, "GBufferNormal");
		RGTextureNode* albedoNode = rg.CreateTextureNode(m_GBufferAlbedo, "GBufferAlbedo");
		RGTextureNode* depthNode = rg.CreateTextureNode(m_Depth, "DepthBuffer");
		RGTextureNode* backBufferNode = rg.CreateTextureNode(backBuffer, "BackBuffer");

		// ========= create pass nodes ==============
		// gBuffer node
		Rect renderArea = { 0, 0, m_CacheWindowSize.w, m_CacheWindowSize.h };
		RGRenderNode* gBufferNode = rg.CreateRenderNode("GBufferPass");
		gBufferNode->SetArea(renderArea);
		gBufferNode->WriteColorTarget(normalNode, 0);
		gBufferNode->WriteColorTarget(albedoNode, 1);
		gBufferNode->WriteDepthTarget(depthNode);
		gBufferNode->SetTask([this, drawCallContext](RHICommandBuffer* cmd) {
			cmd->BindGraphicsPipeline(MeshGBufferPSO);
			drawCallContext->ExecuteDraCall(EDrawCallQueueType::BasePass, cmd);
		});
		// deferred lighting
		RGRenderNode* deferredLightingNode = rg.CreateRenderNode("DeferredLightingPass");
		deferredLightingNode->ReadSRV(normalNode);
		deferredLightingNode->ReadSRV(albedoNode);
		deferredLightingNode->ReadSRV(depthNode);
		deferredLightingNode->WriteColorTarget(backBufferNode, 0);
		deferredLightingNode->SetArea(renderArea);
		deferredLightingNode->SetTask([this, drawCallContext](RHICommandBuffer* cmd) {
			cmd->BindGraphicsPipeline(m_DeferredLightingPSO.Get());
			drawCallContext->ExecuteDraCall(EDrawCallQueueType::DeferredLighting, cmd);
			cmd->SetShaderParam(0, 2, RHIShaderParam::Texture(m_GBufferNormal));
			cmd->SetShaderParam(0, 3, RHIShaderParam::Texture(m_GBufferAlbedo));
			cmd->SetShaderParam(0, 4, RHIShaderParam::Texture(m_Depth, ETextureSRVType::Depth, {}));
			cmd->SetShaderParam(0, 5, RHIShaderParam::Sampler(DefaultResources::Instance()->GetDefaultSampler(ESamplerFilter::Point, ESamplerAddressMode::Clamp)));
			cmd->Draw(6, 1, 0, 0);

			// draw imgui
			ImGuiRHI::Instance()->RenderDrawData(cmd);
		});
		// TODO post process

		fence->Reset();
		deferredLightingNode->InsertFence(fence);
		// present
		RGPresentNode* presentNode = rg.CreatePresentNode();
		presentNode->SetPrevNode(backBufferNode);
		presentNode->SetTask([]() { RHI::Instance()->GetViewport()->Present(); });

		// ========= run the graph ==============
		CmdPool* cmdPool = &m_CmdPools[frameIndex];
		cmdPool->Reset();
		rg.Run(cmdPool);
		cmdPool->GC();
	}

	void ViewportRenderer::WaitAllFence() {
		for(auto& fence: m_Fences) {
			fence->Wait();
		}
	}

	void ViewportRenderer::WaitCurrentFence() {
		RHIFence* fence = m_Fences[FrameCounter::GetFrame() % RHI_FRAME_IN_FLIGHT_MAX].Get();
		fence->Wait();
	}

	void ViewportRenderer::OnWindowSizeChanged(uint32 w, uint32 h) {
		if(w != m_CacheWindowSize.w || h != m_CacheWindowSize.h) {
			m_CacheWindowSize = { w, h };
			m_SizeDirty = true;
		}
	}

	void ViewportRenderer::CreateTextures() {
		RHI* r = RHI::Instance();
		RHITextureDesc desc = RHITextureDesc::Texture2D();
		desc.Flags = TEXTURE_FLAG_COLOR_TARGET | TEXTURE_FLAG_SRV;
		desc.Width = m_CacheWindowSize.w;
		desc.Height = m_CacheWindowSize.h;
		desc.Format = s_NormalFormat;
		m_GBufferNormal = r->CreateTexture(desc);

		desc.Format = s_AlbedoFormat;
		m_GBufferAlbedo = r->CreateTexture(desc);

		desc.Format = RHI::Instance()->GetDepthFormat();
		desc.Flags = TEXTURE_FLAG_DEPTH_TARGET | TEXTURE_FLAG_STENCIL | TEXTURE_FLAG_SRV;
		m_Depth = r->CreateTexture(desc);

		desc.Format = ERHIFormat::R8G8B8A8_UNORM;
		desc.Flags = TEXTURE_FLAG_COLOR_TARGET | TEXTURE_FLAG_SRV;
	}

	bool ViewportRenderer::SizeValid() const {
		return m_CacheWindowSize.w > 0 && m_CacheWindowSize.h > 0;
	}

}
