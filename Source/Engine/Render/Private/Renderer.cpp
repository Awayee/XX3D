#include "Render/Public/Renderer.h"
#include "System/Public/FrameCounter.h"
#include "RHI/Public/ImGuiRHI.h"

namespace Render {
	Renderer::Renderer() {
		m_Cmd = RHI::Instance()->CreateCommandBuffer();
	}

	Renderer::~Renderer() {
		m_Cmd.Reset();
	}

	Renderer::Renderer(Renderer&& rhs) noexcept : m_Cmd(MoveTemp(rhs.m_Cmd)) {
	}

	Renderer& Renderer::operator=(Renderer&& rhs) noexcept{
		m_Cmd = MoveTemp(rhs.m_Cmd);
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
		// initialize the render area
		USize2D vpSize = r->GetViewport()->GetSize();
		m_RenderArea = { 0, 0, vpSize.w, vpSize.h };
	}

	PresentRenderer::~PresentRenderer() {
	}

	void PresentRenderer::SetRenderArea(IURect area) {
		m_RenderArea = area;
	}

	void PresentRenderer::SetDepthTarget(RHITexture* depthTarget) {
		m_DepthTarget = depthTarget;
	}

	void PresentRenderer::Execute(DrawCallQueue& queue) {
		uint32 resIndex = FrameCounter::GetFrame() % RHI_MAX_FRAME_IN_FLIGHT;
		RHICommandBuffer* cmd = m_FrameResources[resIndex].Cmd.Get();
		RHIFence* fence = m_FrameResources[resIndex].Fence.Get();

		// Wait fence before rendering.
		fence->Wait();
		fence->Reset();

		// Acquire next back buffer and transition texture layout.
		auto r = RHI::Instance();
		RHITexture* backBuffer = r->GetViewport()->AcquireBackBuffer();
		if (!backBuffer) {
			LOG_WARNING("[PresentRenderer::Execute] Could not get back buffer!");
			return;
		}

		// Prepare the render pass.
		cmd->Reset();
		cmd->PipelineBarrier(backBuffer, {}, EResourceState::Unknown, EResourceState::RenderTarget);
		RHIRenderPassInfo info;
		info.ColorTargets[0].Target = backBuffer;
		info.DepthStencilTarget.Target = m_DepthTarget;
		info.RenderArea = m_RenderArea;

		// Record the rendering
		cmd->BeginRendering(info);
		for(auto& drawCall: queue) {
			drawCall.Execute(cmd);
		}
		cmd->EndRendering();

		// Present
		cmd->PipelineBarrier(backBuffer, {}, EResourceState::RenderTarget, EResourceState::Present);
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
