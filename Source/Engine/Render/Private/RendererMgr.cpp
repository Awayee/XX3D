#include "Render/Public/Renderer.h"
#include "Render/Public/MeshRender.h"
#include "Render/Public/DefaultResource.h"
#include "System/Public/FrameCounter.h"
#include "RHI/Public/ImGuiRHI.h"
#include "Window/Public/EngineWindow.h"

namespace Render {

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

	ViewportRenderer::ViewportRenderer() {
		// Create fences
		for(uint32 i=0; i<RHI_FRAME_IN_FLIGHT_MAX; ++i) {
			m_Fences[i] = RHI::Instance()->CreateFence(true);
		}
		const TStaticArray<ERHIFormat, 2> formats = { s_NormalFormat, s_AlbedoFormat };
		MeshGBufferPSO = CreateMeshGBufferRenderPSO(formats, RHI::Instance()->GetDepthFormat());
		m_DeferredLightingPSO = CreateDeferredLightingPSO(s_ColorFormat);
	}

	void ViewportRenderer::SetRenderArea(const Rect& renderArea) {
		m_RenderArea = renderArea;
		CreateTextures();
	}

	void ViewportRenderer::Execute(DrawCallContext* drawCallContext) {
		if(!IsRenderAreaValid()) {
			return;
		}
		const uint32 frameIndex = FrameCounter::GetFrame() % RHI_FRAME_IN_FLIGHT_MAX;
		RHIFence* fence = m_Fences[frameIndex].Get();

		// Wait fence before rendering.
		fence->Wait();
		fence->Reset();

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
		RGRenderNode* gBufferNode = rg.CreateRenderNode("GBufferPass");
		gBufferNode->SetArea(m_RenderArea);
		gBufferNode->WriteColorTarget(normalNode, 0);
		gBufferNode->WriteColorTarget(albedoNode, 1);
		gBufferNode->WriteDepthTarget(depthNode);
		gBufferNode->SetTask([this, drawCallContext](RHICommandBuffer* cmd) {
			// TODO fix the errors
			//cmd->BindGraphicsPipeline(MeshGBufferPSO);
			//auto& drawCallQueue = drawCallContext->GetDrawCallQueue(EDrawCallQueueType::BasePass);
			//for(auto& drawCall: drawCallQueue) {
			//	drawCall.Execute(cmd);
			//}
		});
		// deferred lighting
		RGRenderNode* deferredLightingNode = rg.CreateRenderNode("DeferredLightingPass");
		deferredLightingNode->ReadSRV(normalNode);
		deferredLightingNode->ReadSRV(albedoNode);
		deferredLightingNode->ReadSRV(depthNode);
		deferredLightingNode->WriteColorTarget(backBufferNode, 0);
		deferredLightingNode->SetArea(m_RenderArea);
		deferredLightingNode->SetTask([this](RHICommandBuffer* cmd) {
			cmd->BindGraphicsPipeline(m_DeferredLightingPSO.Get());
			cmd->SetShaderParameter(0, 2, RHIShaderParam::Texture(m_GBufferNormal));
			cmd->SetShaderParameter(0, 3, RHIShaderParam::Texture(m_GBufferAlbedo));
			cmd->SetShaderParameter(0, 4, RHIShaderParam::Texture(m_Depth, ETextureSRVType::Depth, {}));
			cmd->SetShaderParameter(0, 5, RHIShaderParam::Sampler(DefaultResources::Instance()->GetDefaultSampler(ESamplerFilter::Point, ESamplerAddressMode::Clamp)));
			//cmd->Draw(6, 1, 0, 0);
			// draw imgui TODO put into post process pass
			ImGuiRHI::Instance()->RenderDrawData(cmd);
		});
		// TODO post process
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
			fence->Reset();
		}
	}

	void ViewportRenderer::WaitCurrentFence() {
		RHIFence* fence = m_Fences[FrameCounter::GetFrame() % RHI_FRAME_IN_FLIGHT_MAX].Get();
		fence->Wait();
		fence->Reset();
	}

	void ViewportRenderer::CreateTextures() {
		RHI* r = RHI::Instance();
		RHITextureDesc desc = RHITextureDesc::Texture2D();
		desc.Flags = TEXTURE_FLAG_COLOR_TARGET | TEXTURE_FLAG_SRV;
		desc.Width = m_RenderArea.w;
		desc.Height = m_RenderArea.h;
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

	bool ViewportRenderer::IsRenderAreaValid() const {
		return m_RenderArea.w > 0 && m_RenderArea.h > 0;
	}

}
