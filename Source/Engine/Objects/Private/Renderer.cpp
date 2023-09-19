#include "Objects/Public/Renderer.h"
#include "Resource/Public/Config.h"
#include "Render/Public/RenderMacro.h"
#include "Window/Public/ImGuiImpl.h"
#include "Window/Public/Wnd.h"
#include "Objects/Public/RenderScene.h"
#include "Objects/Public/Camera.h"
#include "Objects/Public/Material.h"
#include "Asset/Public/AssetLoader.h"

namespace Engine {


	void Renderer::CreateRenderResources()
	{
		// create command buffers
		m_CommandBuffers.Resize(MAX_FRAMES_IN_FLIGHT);
		for (uint8 i = 0; i < m_CommandBuffers.Size(); ++i) {
			m_CommandBuffers[i] = RHI::Instance()->AllocateCommandBuffer(Engine::COMMAND_BUFFER_LEVEL_PRIMARY);
		}
		m_PresentPass.reset(new DeferredLightingPass());
		// create descriptor set
		m_DeferredLightingDescs = RHI::Instance()->AllocateDescriptorSet(DescsMgr::Get(DESCS_DEFERRED_LIGHTING));
		m_DeferredLightingDescs->SetUniformBuffer(0, RenderScene::GetDefaultScene()->m_CameraUniform.Buffer);
		m_DeferredLightingDescs->SetUniformBuffer(1, RenderScene::GetDefaultScene()->m_LightUniform.Buffer);
		m_DeferredLightingDescs->SetInputAttachment(2, m_PresentPass->GetAttachment(DeferredLightingPass::ATTACHMENT_NORMAL));
		m_DeferredLightingDescs->SetInputAttachment(3, m_PresentPass->GetAttachment(DeferredLightingPass::ATTACHMENT_ALBEDO));
		m_DeferredLightingDescs->SetInputAttachment(4, m_PresentPass->GetAttachment(DeferredLightingPass::ATTACHMENT_DEPTH));
	}

	void Renderer::ResizeRenderArea() {
		GET_RHI(rhi);
		if (m_RenderArea.w > 0 && m_RenderArea.h > 0) {
			m_GBufferPipeline.reset(new GBufferPipeline(m_PresentPass.get(), DeferredLightingPass::SUBPASS_BASE, m_RenderArea));
			m_DeferredLightingPipeline.reset(new DeferredLightingPipeline(m_PresentPass.get(), DeferredLightingPass::SUBPASS_DEFERRED_LIGHTING, m_RenderArea));
			RenderScene::GetDefaultScene()->GetMainCamera()->SetAspect(m_RenderArea.w / (float)m_RenderArea.h);
		}
		else {
			m_GBufferPipeline.reset();
			m_GBufferPipeline.reset();
		}
	}


	void Renderer::OnWindowSizeChanged(uint32 w, uint32 h) {
		m_WindowAvailable = (0 != w && 0 != h);
		if (!m_WindowAvailable) {
			return;
		}
		RHI::Instance()->ResizeSwapchain(w, h);
		CreateRenderResources();
		m_RenderAreaDirty = true;
	}

	RFence* Renderer::GetCurrentFence() {
		return m_Fences[m_CurrentFrameIndex];
	}

	Renderer::Renderer(Engine::Wnd* window){
		auto& size = window->GetWindowSize();
		DescsMgr::Initialize();
		SamplerMgr::Initialize();
		SetRenderArea({ 0, 0, size.w, size.h });
		CreateRenderResources();

		// sync
		m_Fences.Resize(MAX_FRAMES_IN_FLIGHT);
		for (uint8 i = 0; i < m_Fences.Size(); ++i) {
			m_Fences[i] = RHI::Instance()->CreateFence();
		}

		// register window func
		window->RegisterOnWindowSizeFunc([this](int w, int h) {
			WaitGPU();
			this->OnWindowSizeChanged((uint32)w, (uint32)h);
		});

	}
	Renderer::~Renderer()
	{
		// wait queue before disabled
		WaitGPU();

		GET_RHI(rhi);
		ImGuiRelease();
		RenderScene::Clear();
		DescsMgr::Release();
		SamplerMgr::Release();
		MaterialMgr::Release();
		TextureMgr::Release();
		m_GBufferPipeline.reset();
		m_DeferredLightingPipeline.reset();
		m_PresentPass.reset();

		for(auto& cmd: m_CommandBuffers) {
			rhi->FreeCommandBuffer(cmd);
		}
		m_CommandBuffers.Clear();
		for(auto& fence: m_Fences) {
			rhi->DestroyFence(fence);
		}
		m_Fences.Clear();
	}

	void Renderer::Tick(){
		// window is hided, pause rendering
		if(!m_WindowAvailable) {
			return;
		}
		//wait
		m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
		auto mainScene = RenderScene::GetDefaultScene();
		mainScene->Update();

		GET_RHI(rhi);
		// begin render pass;
		rhi->WaitForFence(GetCurrentFence());

		if(m_RenderAreaDirty) {
			ResizeRenderArea();
			m_RenderAreaDirty = false;
			return;
		}

		int swapchainImageIndex = rhi->PreparePresent(m_CurrentFrameIndex);
		if(-1 == swapchainImageIndex) {
			OnWindowSizeChanged(0, 0);
			return;
		}

		rhi->ResetFence(GetCurrentFence());

		Engine::RCommandBuffer* cmd = m_CommandBuffers[m_CurrentFrameIndex];
		cmd->Begin(0);

		m_PresentPass->SetImageIndex(swapchainImageIndex);
		m_PresentPass->Begin(cmd);

		// 1. render main viewport
		if(RENDER_DEFERRED == GetConfig().RenderPath) {
			// render g buffer pass
			if(m_GBufferPipeline) {
				m_GBufferPipeline->Bind(cmd);
				mainScene->RenderGBuffer(cmd, m_GBufferPipeline->GetLayout());
			}
			cmd->NextSubpass();
			// render deferred pass
			if(m_DeferredLightingPipeline) {
				m_DeferredLightingPipeline->Bind(cmd);
				cmd->BindDescriptorSet(m_DeferredLightingPipeline->GetLayout(), m_DeferredLightingDescs, 0, Engine::PIPELINE_GRAPHICS);
				cmd->Draw(6, 1, 0, 0);
			}

		}

		// 2. render imgui
		Engine::ImGuiRenderDrawData(cmd);

		cmd->EndRenderPass();
		cmd->End();
		int res = rhi->QueueSubmitPresent(cmd, m_CurrentFrameIndex, GetCurrentFence());
		if(-1 == res) {
			OnWindowSizeChanged(0, 0);
		}
	}

	void Renderer::InitUIPass(void(*InitializeFunc)())
	{
		// the last subpass is for ui
		Engine::ImGuiInitialize(m_PresentPass->GetRHIPass(), 1);
		InitializeFunc();
		GET_RHI(rhi);
		// upload font
		rhi->ImmediateCommit([](Engine::RCommandBuffer* cmd) {
			Engine::ImGuiCreateFontsTexture(cmd);
		});
		Engine::ImGuiDestroyFontUploadObjects();
	}

	void Renderer::SetRenderArea(const URect& area){
		m_RenderArea = area;
		m_RenderAreaDirty = true;
	}

	void Renderer::WaitGPU() {
		for(auto& fence: m_Fences) {
			RHI::Instance()->WaitForFence(fence);
		}
	}
}
