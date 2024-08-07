#include "Render/Public/Renderer.h"
#include "System/Public/FrameCounter.h"
#include "RHI/Public/ImGuiRHI.h"
#include "Window/Public/EngineWindow.h"

namespace Render {
	Renderer::Renderer() {
		m_Cmd = RHI::Instance()->CreateCommandBuffer();
	}

	Renderer::~Renderer() {
		m_Cmd.Reset();
	}

	Renderer::Renderer(Renderer&& rhs) noexcept : m_Cmd(MoveTemp(rhs.m_Cmd)), m_RenderPass(rhs.m_RenderPass), m_RenderArea(rhs.m_RenderArea) {
	}

	Renderer& Renderer::operator=(Renderer&& rhs) noexcept{
		m_Cmd = MoveTemp(rhs.m_Cmd);
		m_RenderPass = rhs.m_RenderPass;
		m_RenderArea = rhs.m_RenderArea;
		return *this;
	}

	void Renderer::SetColorTarget(RHITexture* renderTarget){
		m_RenderPass.ColorTargets[0].Target = renderTarget;
	}

	void Renderer::SetColorTargets(TConstArrayView<RHITexture*> renderTargets){
		ASSERT(renderTargets.Size() <= RHI_MAX_RENDER_TARGET_NUM, "Too many render targets!");
		for(uint32 i=0; i<renderTargets.Size(); ++i) {
			m_RenderPass.ColorTargets[i].Target = renderTargets[i];
		}
	}

	void Renderer::SetDepthTarget(RHITexture* depthTarget) {
		m_RenderPass.DepthStencilTarget.Target = depthTarget;
	}

	void Renderer::SetRenderArea(IURect area) {
		m_RenderArea = area;
	}

	void Renderer::Execute(DrawCallQueue& queue) {
		auto r = RHI::Instance();
		m_Cmd->Reset();
		m_Cmd->BeginRendering(m_RenderPass);
		for(auto& dc: queue) {
			dc.Execute(m_Cmd.Get());
		}
		m_Cmd->EndRendering();
		r->SubmitCommandBuffer(m_Cmd.Get(), nullptr, false);
	}

	PresentRenderer::PresentRenderer(): m_DepthTarget(nullptr) {
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

	PresentRenderer::~PresentRenderer() {
	}

	void PresentRenderer::SetDepthTarget(RHITexture* depthTarget) {
		m_DepthTarget = depthTarget;
	}

	void PresentRenderer::Execute(DrawCallQueue& queue) {
		const uint32 resIndex = FrameCounter::GetFrame() % RHI_MAX_FRAME_IN_FLIGHT;
		RHICommandBuffer* cmd = m_FrameResources[resIndex].Cmd.Get();
		RHIFence* fence = m_FrameResources[resIndex].Fence.Get();

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
		cmd->TransitionTextureState(backBuffer, EResourceState::Unknown, EResourceState::RenderTarget, {});
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
		cmd->TransitionTextureState(backBuffer, EResourceState::RenderTarget, EResourceState::Present, {});
		r->SubmitCommandBuffer(cmd, fence, true);
		r->GetViewport()->Present();
	}

	void PresentRenderer::WaitQueue() {
		// The fences must be signaled before destruction.
		for (auto& res : m_FrameResources) {
			res.Fence->Wait();
		}
	}

	void RendererMgr::Update() {
		DrawCallQueue queue;
		// TODO test
		queue.EmplaceBack().ResetFunc([](RHICommandBuffer* cmd) {
			ImGuiRHI::Instance()->RenderDrawData(cmd);
		});
		m_PresentRenderer.Execute(queue);
	}

	void RendererMgr::WaitQueue() {
		m_PresentRenderer.WaitQueue();
	}

	RendererMgr::RendererMgr() {
	}

	RendererMgr::~RendererMgr() {
	}

}
