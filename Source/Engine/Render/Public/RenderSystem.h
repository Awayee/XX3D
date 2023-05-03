#pragma once
#include "Core/Public/SmartPointer.h"
#include "Core/Public/BaseStructs.h"
#include "RHI/Public/RHI.h"
#include "RenderResources.h"

namespace Engine {
	class Wnd;

	enum EPassType {
		PASS_PRESENT,   // main render
		//PASS_BASE, // normal
		PASS_COUNT,
	};

	class RenderData {
		TVector<Primitive> Primitives;
	};

	class RenderSystem {
	private:
		bool m_Enable{ false };
		//Render Passes
		TUniquePtr<DeferredLightingPass> m_PresentPass;
		// command buffers
		TVector<Engine::RCommandBuffer*> m_CommandBuffers;
		// Render pipelines
		TUniquePtr<GBufferPipeline> m_GBufferPipeline;
		TUniquePtr<DeferredLightingPipeline> m_DeferredLightingPipeline;
		Engine::RDescriptorSet* m_DeferredLightingDescs;

		uint8_t m_CurrentFrameIndex{0};
		bool m_WindowAvailable{ true };
		URect m_RenderArea;
		bool m_RenderAreaDirty{ false };

	public:
		RenderSystem() = default;
		RenderSystem(Engine::Wnd* window);
		~RenderSystem();
		void SetEnable(bool enable);
		void Tick();
		void InitUIPass() const;
		void SetRenderArea(const URect& area);

	private:
		void CreateRenderResources();
		void ResizeRenderArea();
		void OnWindowSizeChanged(uint32 w, uint32 h);
	};
}
