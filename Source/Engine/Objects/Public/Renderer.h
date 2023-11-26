#pragma once
#include "Core/Public/BaseStructs.h"
#include "RHI/Public/RHI.h"
#include "Render/Public/RenderReses.h"

namespace Engine {
	class Wnd;

	enum EPassType {
		PASS_PRESENT,   // main render
		//PASS_BASE, // normal
		PASS_COUNT,
	};

	class Renderer {
	private:
		bool m_Enable{ false };
		//Render Passes
		TUniquePtr<DeferredLightingPass> m_PresentPass;
		// command buffers
		TVector<Engine::RHICommandBuffer*> m_CommandBuffers;
		// Render pipelines
		TUniquePtr<GBufferPipeline> m_GBufferPipeline;
		TUniquePtr<DeferredLightingPipeline> m_DeferredLightingPipeline;
		Engine::RDescriptorSet* m_DeferredLightingDescs;

		uint8_t m_CurrentFrameIndex{0};
		bool m_WindowAvailable{ true };
		URect m_RenderArea;
		bool m_RenderAreaDirty{ false };
		TVector<RHIFence*> m_Fences;

	private:
		void CreateRenderResources();
		void ResizeRenderArea();
		void OnWindowSizeChanged(uint32 w, uint32 h);
		RHIFence* GetCurrentFence();

	public:
		Renderer(Engine::Wnd* window);
		~Renderer();
		void Tick();
		void InitUIPass(void(*InitializeFunc)());
		void SetRenderArea(const URect& area);
		void WaitGPU();
	};
}
