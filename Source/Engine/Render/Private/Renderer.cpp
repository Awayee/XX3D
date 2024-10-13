#include "Render/Public/Renderer.h"
#include"Render/Public/DefaultResource.h"
#include "System/Public/Timer.h"
#include "Window/Public/EngineWindow.h"

namespace Render {

	RHICommandBuffer* CmdPool::CmdArray::Get() {
		if (AllocIndex >= Cmds.Size()) {
			for (uint32 i = Cmds.Size(); i <= AllocIndex; ++i) {
				Cmds.PushBack(RHI::Instance()->CreateCommandBuffer(QueueType));
			}
		}
		return Cmds[AllocIndex++].Get();
	}

	void CmdPool::CmdArray::Reset() {
		AllocIndex = 0;
	}

	void CmdPool::CmdArray::GC() {
		if (!Cmds.IsEmpty()) {
			for (uint32 i = Cmds.Size() - 1; i > AllocIndex; --i) {
				Cmds.PopBack();
			}
		}
	}

	CmdPool::CmdPool() {
		for(uint8 i=0; i<EnumCast(EQueueType::Count); ++i) {
			m_CmdArrays[i].QueueType = (EQueueType)i;
		}
	}

	RHICommandBuffer* CmdPool::GetCmd(EQueueType queue) {
		RHICommandBuffer* cmd = m_CmdArrays[EnumCast(queue)].Get();
		cmd->Reset();
		return cmd;

	}

	void CmdPool::Reset() {
		for(auto& cmdArray: m_CmdArrays) {
			cmdArray.Reset();
		}
	}

	void CmdPool::GC() {
		for(auto& cmdArray: m_CmdArrays) {
			cmdArray.GC();
		}
	}

	void Renderer::Run() {
		if(!SizeValid()) {
			return;
		}
		const uint32 frameIndex = Engine::Timer::GetFrame() % RHI_FRAME_IN_FLIGHT_MAX;
		RHIFence* fence = m_Fences[frameIndex].Get();

		if (m_SizeDirty) {
			WaitAllFence();
			RHI::Instance()->GetViewport()->SetSize(m_CacheWindowSize);
			m_SizeDirty = false;
		}

		RHIViewport* viewport = RHI::Instance()->GetViewport();
		if (!viewport->PrepareBackBuffer()) {
			return;
		}

		RHITexture* backBuffer = viewport->GetBackBuffer();
		RenderGraph rg;
		// get back buffer
		RGTextureNode* backBufferNode = rg.CreateTextureNode(backBuffer, "BackBuffer");
		// render scene
		if(m_SceneRenderer) {
			m_SceneRenderer->Render(rg, backBufferNode);
		}
		// fence after back buffer written
		rg.InsertFence(fence, backBufferNode);
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

	void Renderer::WaitAllFence() {
		for(auto& fence: m_Fences) {
			fence->Wait();
		}
	}

	void Renderer::OnWindowSizeChanged(uint32 w, uint32 h) {
		if(w != m_CacheWindowSize.w || h != m_CacheWindowSize.h) {
			m_CacheWindowSize = { w, h };
			m_SizeDirty = true;
		}
	}

	Renderer::Renderer() : m_SizeDirty(false) {
		// Create fences
		for (uint32 i = 0; i < RHI_FRAME_IN_FLIGHT_MAX; ++i) {
			m_Fences[i] = RHI::Instance()->CreateFence(true);
			auto name = StringFormat("RenderCompleteFence%u", i);
			m_Fences[i]->SetName(name.c_str());
		}

		// init window area
		m_CacheWindowSize = Engine::EngineWindow::Instance()->GetWindowSize();
		Engine::EngineWindow::Instance()->RegisterOnWindowSizeFunc([this](uint32 w, uint32 h) {OnWindowSizeChanged(w, h); });
	}

	Renderer::~Renderer() {
		WaitAllFence();
	}

	bool Renderer::SizeValid() const {
		return m_CacheWindowSize.w > 0 && m_CacheWindowSize.h > 0;
	}

}
