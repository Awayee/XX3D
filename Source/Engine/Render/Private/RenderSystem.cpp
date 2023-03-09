#include "Render/Public/RenderSystem.h"
#include "Resource/Public/Config.h"
#include "RenderMacro.h"
#include "Render/Public/ImGuiImpl.h"
#include "Render/Public/WindowSystem.h"
#include "Render/Public/RenderScene.h"
#include "Render/Public/Camera.h"
#include "Render/Public/Material.h"

namespace Engine {
	RenderSystem::RenderSystem(WindowSystemBase* window){
		int windowWidth, windowHeight;
		window->GetWindowSize(&windowWidth, &windowHeight);
		// init rhi
		{
			RHI::RSInitInfo initInfo;
			initInfo.applicationName = PROJECT_NAME;
			initInfo.enableDebug =
#ifdef _DEBUG
				true
#else
				false
#endif
			;
			initInfo.enableGeometryShader = true;
			initInfo.windowWidth = windowWidth;
			initInfo.windowHeight = windowHeight;
			initInfo.windowHandle = window->GetWindowHandle();
			initInfo.maxFramesInFlight = MAX_FRAMES_IN_FLIGHT;
			RHI::GetInstance()->Initialize(&initInfo);
		}

		DescsMgr::Initialize();
		SamplerMgr::Initialize();
		SetRenderArea({ 0, 0, static_cast<uint32>(windowWidth), static_cast<uint32>(windowHeight) });
		CreateRenderResources();
		// register window func
		window->RegisterOnWindowSizeFunc([this](int w, int h) {
			this->OnWindowSizeChanged((uint32)w, (uint32)h);
		});

	}
	RenderSystem::~RenderSystem()
	{
		RenderScene::Clear();
		DescsMgr::Release();
		SamplerMgr::Release();
		MaterialMgr::Release();
		TextureMgr::Release();
		m_GBufferPipeline.reset();
		m_DeferredLightingPipeline.reset();
		m_PresentPass.reset();

		GET_RHI(rhi);
		for(auto& cmd: m_CommandBuffers) {
			rhi->FreeCommandBuffer(cmd);
		}

		rhi->Release();
	}

	void RenderSystem::SetEnable(bool enable)
	{
		m_Enable = enable;
		if(!enable) {
			// wait queue before disabled
			RHI_INSTANCE->WaitGraphicsQueue();
		}
	}

	void RenderSystem::Tick(){
		if(!m_Enable) {
			return;
		}

		// window is hided, pause rendering
		if(!m_WindowAvailable) {
			return;
		}

		auto mainScene = RenderScene::GetDefaultScene();
		mainScene->Update();

		if(m_RenderAreaDirty) {
			ResizeRenderArea();
			m_RenderAreaDirty = false;
			return;
		}

		GET_RHI(rhi);
		// begin render pass;
		int swapchainImageIndex = rhi->PreparePresent(m_CurrentFrameIndex);
		if(-1 == swapchainImageIndex) {
			OnWindowSizeChanged(0, 0);
			return;
		}
		RHI::RCommandBuffer* cmd = m_CommandBuffers[m_CurrentFrameIndex];
		cmd->Begin(0);

		m_PresentPass->SetImageIndex(swapchainImageIndex);
		m_PresentPass->Begin(cmd);

		// 1. render main viewport
		if(RENDER_DEFERRED == GetConfig()->GetRenderPath()) {
			// render g buffer pass
			if(m_GBufferPipeline) {
				m_GBufferPipeline->Bind(cmd);
				mainScene->RenderGBuffer(cmd, m_GBufferPipeline->GetLayout());
			}
			cmd->NextSubpass();
			// render deferred pass
			if(m_DeferredLightingPipeline) {
				m_DeferredLightingPipeline->Bind(cmd);
				cmd->BindDescriptorSet(m_DeferredLightingPipeline->GetLayout(), m_DeferredLightingDescs, 0, RHI::PIPELINE_GRAPHICS);
				cmd->Draw(6, 1, 0, 0);
			}

		}

		// 2. render imgui
		Engine::ImGuiRenderDrawData(cmd);

		cmd->EndRenderPass();
		cmd->End();
		int res = rhi->QueueSubmitPresent(cmd, m_CurrentFrameIndex);
		if(-1 == res) {
			OnWindowSizeChanged(0, 0);
		}
		m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;

	}

	void RenderSystem::InitUIPass() const
	{
		// the last subpass is for ui
		Engine::ImGuiInitialize(m_PresentPass->GetRHIPass(), 1);
		GET_RHI(rhi);
		// upload font
		rhi->ImmediateCommit([](RHI::RCommandBuffer* cmd) {
			ImGuiCreateFontsTexture(cmd);
			});
		ImGuiDestroyFontUploadObjects();
	}

	void RenderSystem::SetRenderArea(const URect& area){
		m_RenderArea = area;
		m_RenderAreaDirty = true;
	}


	void RenderSystem::CreateRenderResources()
	{
		// create command buffers
		m_CommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		for (uint8 i = 0; i < m_CommandBuffers.size(); ++i) {
			m_CommandBuffers[i] = RHI_INSTANCE->AllocateCommandBuffer(RHI::COMMAND_BUFFER_LEVEL_PRIMARY);
		}
		m_PresentPass.reset(new DeferredLightingPass());
		// create descriptor set
		m_DeferredLightingDescs = RHI_INSTANCE->AllocateDescriptorSet(DescsMgr::Get(DESCS_DEFERRED_LIGHTING));
		m_DeferredLightingDescs->UpdateUniformBuffer(0, RenderScene::GetDefaultScene()->m_CameraUniform.Buffer);
		m_DeferredLightingDescs->UpdateUniformBuffer(1, RenderScene::GetDefaultScene()->m_LightUniform.Buffer);
		m_DeferredLightingDescs->UpdateInputAttachment(2, m_PresentPass->GetAttachment(DeferredLightingPass::ATTACHMENT_NORMAL));
		m_DeferredLightingDescs->UpdateInputAttachment(3, m_PresentPass->GetAttachment(DeferredLightingPass::ATTACHMENT_ALBEDO));
		m_DeferredLightingDescs->UpdateInputAttachment(4, m_PresentPass->GetAttachment(DeferredLightingPass::ATTACHMENT_DEPTH));
	}

	void RenderSystem::ResizeRenderArea(){
		GET_RHI(rhi);
		rhi->WaitGraphicsQueue();
		if(m_RenderArea.w > 0 && m_RenderArea.h > 0) {
			m_GBufferPipeline.reset(new GBufferPipeline(m_PresentPass.get(), DeferredLightingPass::SUBPASS_BASE, m_RenderArea));
			m_DeferredLightingPipeline.reset(new DeferredLightingPipeline(m_PresentPass.get(), DeferredLightingPass::SUBPASS_DEFERRED_LIGHTING, m_RenderArea));
		}
		else {
			m_GBufferPipeline.reset();
			m_GBufferPipeline.reset();
		}
	}


	void RenderSystem::OnWindowSizeChanged(uint32 w, uint32 h){
		m_WindowAvailable = (0 != w && 0 != h);
		if(!m_WindowAvailable) {
			return;
		}
		RHI_INSTANCE->WaitGraphicsQueue();
		RHI_INSTANCE->ResizeSwapchain(w, h);
		CreateRenderResources();
		m_RenderAreaDirty = true;
	}
}
