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

	void ViewportRenderer::Run() {
		if(!SizeValid()) {
			return;
		}
		const uint32 frameIndex = FrameCounter::GetFrame() % RHI_FRAME_IN_FLIGHT_MAX;
		RHIFence* fence = m_Fences[frameIndex].Get();

		if (m_SizeDirty) {
			WaitAllFence();
			RHI::Instance()->GetViewport()->SetSize(m_CacheWindowSize);
			m_SizeDirty = false;
		}

		RHIViewport* viewport = RHI::Instance()->GetViewport();
		RHITexture* backBuffer = viewport->AcquireBackBuffer();
		if (!backBuffer) {
			return;
		}

		const Rect renderArea = { 0, 0, m_CacheWindowSize.w, m_CacheWindowSize.h };
		RenderGraph rg;
		RGTextureNode* backBufferNode = rg.CreateTextureNode(backBuffer, "BackBuffer");
		RGRenderNode* imGuiNode = rg.CreateRenderNode("ImGui");
		if(m_RenderScene) {
			if(RGTextureNode* renderTargetNode = m_RenderScene->Render(rg)) {
				imGuiNode->ReadSRV(renderTargetNode);
			}
		}
		imGuiNode->WriteColorTarget(backBufferNode, 0);
		imGuiNode->SetArea(renderArea);
		imGuiNode->SetTask([this](RHICommandBuffer* cmd) {
			// draw imgui
			ImGuiRHI::Instance()->RenderDrawData(cmd);
		});
		imGuiNode->InsertFence(fence);
		// present
		RGPresentNode* presentNode = rg.CreatePresentNode();
		presentNode->SetPrevNode(backBufferNode);
		presentNode->SetTask([]() { RHI::Instance()->GetViewport()->Present(); });

		// ========= run the graph ==============
		fence->Reset();
		CmdPool* cmdPool = &m_CmdPools[frameIndex];
		cmdPool->Reset();
		rg.Run(cmdPool);
		cmdPool->GC();

		// ========= wait next fence for beginning next frame ==============
		RHIFence* nextFence = m_Fences[(frameIndex + 1) % RHI_FRAME_IN_FLIGHT_MAX].Get();
		nextFence->Wait();
	}

	void ViewportRenderer::WaitAllFence() {
		for(auto& fence: m_Fences) {
			fence->Wait();
		}
	}

	void ViewportRenderer::OnWindowSizeChanged(uint32 w, uint32 h) {
		if(w != m_CacheWindowSize.w || h != m_CacheWindowSize.h) {
			m_CacheWindowSize = { w, h };
			m_SizeDirty = true;
		}
	}

	ViewportRenderer::ViewportRenderer() : m_RenderScene(nullptr), m_SizeDirty(false) {
		// Create fences
		for (uint32 i = 0; i < RHI_FRAME_IN_FLIGHT_MAX; ++i) {
			m_Fences[i] = RHI::Instance()->CreateFence(true);
		}

		// init window area
		m_CacheWindowSize = Engine::EngineWindow::Instance()->GetWindowSize();
		Engine::EngineWindow::Instance()->RegisterOnWindowSizeFunc([this](uint32 w, uint32 h) {OnWindowSizeChanged(w, h); });
	}

	ViewportRenderer::~ViewportRenderer() {
		WaitAllFence();
	}

	bool ViewportRenderer::SizeValid() const {
		return m_CacheWindowSize.w > 0 && m_CacheWindowSize.h > 0;
	}

}
