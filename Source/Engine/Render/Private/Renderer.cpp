#include "Render/Public/Renderer.h"
#include "Render/Public/MeshRender.h"
#include "Render/Public/DefaultResource.h"
#include "System/Public/FrameCounter.h"
#include "RHI/Public/ImGuiRHI.h"
#include "Window/Public/EngineWindow.h"

namespace Render {
	TStaticArray<ERHIFormat, 2> BasePass::s_ColorFormats{ERHIFormat::R8G8B8A8_UNORM, ERHIFormat::R8G8B8A8_UNORM};
	BasePass::BasePass() {
		RHI* r = RHI::Instance();
		m_Cmd = r->CreateCommandBuffer();
		USize2D windowSize = Engine::EngineWindow::Instance()->GetWindowSize();
		m_TargetSize = windowSize;
		m_RenderArea = { 0, 0, m_TargetSize.w, m_TargetSize.h };
		ResetTargets();
		// create pipeline
		m_PSO = CreateMeshGBufferRenderPSO(s_ColorFormats, r->GetDepthFormat());
	}

	BasePass::~BasePass() {
	}
	
	void BasePass::SetTargetSize(USize2D size) {
		if(m_TargetSize != size) {
			m_TargetSize = size;
			ResetTargets();
		}
	}

	void BasePass::SetRenderArea(IURect area) {
		m_RenderArea = area;
	}

	void BasePass::Execute(DrawCallQueue& queue) {
		RHIRenderPassInfo renderPassInfo;
		renderPassInfo.ColorTargets[0].Target = m_ColorTargets[0];
		renderPassInfo.ColorTargets[1].Target = m_ColorTargets[1];
		renderPassInfo.DepthStencilTarget.Target = m_DepthTarget;
		renderPassInfo.RenderArea = m_RenderArea;
		m_Cmd->Reset();
		m_Cmd->TransitionTextureState(m_ColorTargets[0], EResourceState::Unknown, EResourceState::RenderTarget);
		m_Cmd->TransitionTextureState(m_ColorTargets[1], EResourceState::Unknown, EResourceState::RenderTarget);
		m_Cmd->TransitionTextureState(m_DepthTarget, EResourceState::Unknown, EResourceState::DepthStencil);
		m_Cmd->BeginRendering(renderPassInfo);
		for(auto& dc: queue) {
			dc.Execute(m_Cmd);
		}
		m_Cmd->EndRendering();
		m_Cmd->TransitionTextureState(m_ColorTargets[0], EResourceState::RenderTarget, EResourceState::ShaderResourceView);
		m_Cmd->TransitionTextureState(m_ColorTargets[1], EResourceState::RenderTarget, EResourceState::ShaderResourceView);
		m_Cmd->TransitionTextureState(m_DepthTarget, EResourceState::DepthStencil, EResourceState::ShaderResourceView);
		RHI::Instance()->SubmitCommandBuffer(m_Cmd, nullptr, false);
	}

	RHITexture* BasePass::GetColorTarget(uint32 i) {
		ASSERT(i < m_ColorTargets.Size(), "");
		return m_ColorTargets[i].Get();
	}

	RHITexture* BasePass::GetDepthTarget() {
		return m_DepthTarget;
	}

	void BasePass::ResetTargets() {
		RHI* r = RHI::Instance();
		RHITextureDesc desc = RHITextureDesc::Texture2D();
		desc.Flags = TEXTURE_FLAG_COLOR_TARGET | TEXTURE_FLAG_SRV;
		desc.Width = m_TargetSize.w;
		desc.Height = m_TargetSize.h;
		desc.Format = s_ColorFormats[0];
		m_ColorTargets[0] = r->CreateTexture(desc);
		desc.Format = s_ColorFormats[1];
		m_ColorTargets[1] = r->CreateTexture(desc);

		desc.Format = RHI::Instance()->GetDepthFormat();
		desc.Flags = TEXTURE_FLAG_DEPTH_TARGET | TEXTURE_FLAG_STENCIL | TEXTURE_FLAG_SRV;
		m_DepthTarget = r->CreateTexture(desc);
	}

	ERHIFormat DeferredLightingPass::s_ColorFormat{ ERHIFormat::R8G8B8A8_UNORM };

	DeferredLightingPass::DeferredLightingPass() {
		RHI* r = RHI::Instance();
		m_Cmd = r->CreateCommandBuffer();
		m_PSO = CreateDeferredLightingPSO(s_ColorFormat);
	}

	DeferredLightingPass::~DeferredLightingPass() {
	}

	void DeferredLightingPass::SetInput(uint32 i, RHITexture* input) {
		CHECK(i < m_InputTargets.Size());
		m_InputTargets[i] = input;
		uint32 width = input->GetDesc().Width;
		uint32 height = input->GetDesc().Height;
		// reset render target
		if(!m_ColorTarget || m_ColorTarget->GetDesc().Width != width || m_ColorTarget->GetDesc().Height != height) {
			RHITextureDesc desc = RHITextureDesc::Texture2D();
			desc.Format = s_ColorFormat;
			desc.Flags = TEXTURE_FLAG_COLOR_TARGET | TEXTURE_FLAG_SRV;
			desc.Width = width;
			desc.Height = height;
			m_ColorTarget = RHI::Instance()->CreateTexture(desc);
		}
	}

	void DeferredLightingPass::SetRenderArea(IURect area) {
		m_RenderArea = area;
	}

	void DeferredLightingPass::Execute() {
		RHIRenderPassInfo info;
		info.ColorTargets[0].Target = m_ColorTarget;
		info.RenderArea = m_RenderArea;
		m_Cmd->Reset();
		m_Cmd->TransitionTextureState(m_ColorTarget, EResourceState::Unknown, EResourceState::RenderTarget);
		m_Cmd->BeginRendering(info);
		m_Cmd->BindGraphicsPipeline(m_PSO);
		m_Cmd->SetShaderParameter(0, 2, RHIShaderParam::Texture(m_InputTargets[0]));
		m_Cmd->SetShaderParameter(0, 3, RHIShaderParam::Texture(m_InputTargets[1]));
		m_Cmd->SetShaderParameter(0, 4, RHIShaderParam::Texture(m_InputTargets[2], ETextureSRVType::Depth, {}));
		RHISampler* pointSampler = DefaultResources::Instance()->GetDefaultSampler(ESamplerFilter::Point, ESamplerAddressMode::Clamp);
		m_Cmd->SetShaderParameter(0, 5, RHIShaderParam::Sampler(pointSampler));
		//m_Cmd->Draw(6, 1, 0, 0);
		m_Cmd->EndRendering();
		m_Cmd->TransitionTextureState(m_ColorTarget, EResourceState::RenderTarget, EResourceState::ShaderResourceView);
		RHI::Instance()->SubmitCommandBuffer(m_Cmd, nullptr, false);
	}

	RHITexture* DeferredLightingPass::GetColorTarget() {
		return m_ColorTarget;
	}

	PresentPass::PresentPass(): m_DepthTarget(nullptr) {
		RHI* r = RHI::Instance();
		// create command buffer
		for (uint32 i = 0; i < RHI_MAX_FRAME_IN_FLIGHT; ++i) {
			m_FrameResources[i].Cmd = r->CreateCommandBuffer();
			m_FrameResources[i].Fence = r->CreateFence(true);
		}
		// Resize the RHI viewport if window resized.
		Engine::EngineWindow::Instance()->RegisterOnWindowSizeFunc([](uint32 w, uint32 h) {
			RHI::Instance()->GetViewport()->SetSize({ w, h });
		});
	}

	PresentPass::~PresentPass() {
	}

	void PresentPass::SetDepthTarget(RHITexture* depthTarget) {
		m_DepthTarget = depthTarget;
	}

	void PresentPass::Execute(DrawCallQueue& queue) {
		const uint32 resIndex = FrameCounter::GetFrame() % RHI_MAX_FRAME_IN_FLIGHT;
		RHICommandBuffer* cmd = m_FrameResources[resIndex].Cmd;
		RHIFence* fence = m_FrameResources[resIndex].Fence;

		// Wait fence before rendering.
		fence->Wait();
		// Acquire next back buffer and transition texture layout.
		auto r = RHI::Instance();
		RHITexture* backBuffer = r->GetViewport()->AcquireBackBuffer();
		if (!backBuffer) {
			return;
		}

		fence->Reset();
		// Prepare the render pass.
		cmd->Reset();
		cmd->TransitionTextureState(backBuffer, EResourceState::Unknown, EResourceState::RenderTarget);
		RHIRenderPassInfo info;
		info.ColorTargets[0].Target = backBuffer;
		info.DepthStencilTarget.Target = m_DepthTarget;
		USize2D viewportSize = r->GetViewport()->GetSize();
		info.RenderArea = { 0, 0, viewportSize.w, viewportSize.h };

		// Record the rendering
		cmd->BeginRendering(info);
		for(auto& drawCall: queue) {
			drawCall.Execute(cmd);
		}
		cmd->EndRendering();

		// Present
		cmd->TransitionTextureState(backBuffer, EResourceState::RenderTarget, EResourceState::Present);
		r->SubmitCommandBuffer(cmd, fence, true);
		r->GetViewport()->Present();
	}

	void PresentPass::WaitAll() {
		// The fences must be signaled before destruction.
		for (auto& res : m_FrameResources) {
			res.Fence->Wait();
		}
		std::unique_ptr<RHICommandBuffer> null = nullptr;
	}

	void PresentPass::WaitCurrent() {
		m_FrameResources[FrameCounter::GetFrame() % RHI_MAX_FRAME_IN_FLIGHT].Fence->Wait();
	}

	void RendererMgr::Update() {
		// wait last frame
		m_PresentRenderer.WaitCurrent();
		// draw scene
		DrawCallQueue sceneDrawCalls;
		m_BasePass.Execute(sceneDrawCalls);
		m_DeferredLightingPass.Execute();
		// draw imgui
		DrawCallQueue queue;
		queue.EmplaceBack().ResetFunc([](RHICommandBuffer* cmd) {
			ImGuiRHI::Instance()->RenderDrawData(cmd);
		});
		m_PresentRenderer.Execute(queue);
	}

	void RendererMgr::WaitQueue() {
		m_PresentRenderer.WaitAll();
	}

	RendererMgr::RendererMgr() {
		RHITexture* normalTex = m_BasePass.GetColorTarget(0);
		RHITexture* albedoTex = m_BasePass.GetColorTarget(1);
		RHITexture* depthTex = m_BasePass.GetDepthTarget();
		m_DeferredLightingPass.SetInput(0, normalTex);
		m_DeferredLightingPass.SetInput(1, albedoTex);
		m_DeferredLightingPass.SetInput(2, depthTex);
		m_DeferredLightingPass.SetRenderArea(m_BasePass.GetRenderArea());
	}

	RendererMgr::~RendererMgr() {
	}

}
