#pragma once
#include "RHI/Public/RHI.h"
#include "Core/Public/TUniquePtr.h"
#include "Render/Public/DrawCall.h"
#include "Core/Public/TArrayView.h"

namespace Render {
	class RendererBase {
	public:
		NON_COPYABLE(RendererBase);
		NON_MOVEABLE(RendererBase);
		virtual ~RendererBase() = default;
		virtual void Execute(DrawCallQueue& queue) = 0;
	};

	class Renderer {
	public:
		NON_COPYABLE(Renderer);
		Renderer();
		~Renderer();
		Renderer(Renderer&& rhs) noexcept;
		Renderer& operator=(Renderer&& rhs) noexcept;
		void SetColorTarget(RHITexture* renderTarget);
		void SetColorTargets(TConstArrayView<RHITexture*> renderTargets);
		void SetDepthTarget(RHITexture* depthTarget);
		void SetRenderArea(IURect area);
		void Execute(DrawCallQueue& queue);
	private:
		RHICommandBufferPtr m_Cmd;
		RHIRenderPassInfo m_RenderPass;
		IURect m_RenderArea;
	};

	// The pass directly render for Swapchain
	class PresentRenderer {
	public:
		NON_COPYABLE(PresentRenderer);
		NON_MOVEABLE(PresentRenderer);
		PresentRenderer();
		~PresentRenderer();
		void SetRenderArea(IURect area);
		void SetDepthTarget(RHITexture* depthTarget);
		void Execute(DrawCallQueue& queue);
		void WaitQueue();
	private:
		struct FrameResource {
			RHICommandBufferPtr Cmd;
			RHIFencePtr Fence;// rendering complete in this frame.
		};
		TStaticArray<FrameResource, RHI_MAX_FRAME_IN_FLIGHT> m_FrameResources;
		RHITexture* m_DepthTarget;
		IURect m_RenderArea;
	};

	class RendererMgr : public TSingleton<RendererMgr> {
		friend TSingleton<RendererMgr>;
	public:
		void Update();
		void WaitQueue();
	private:
		Renderer m_Renderer;
		// The last renderer
		PresentRenderer m_PresentRenderer;
		RendererMgr();
		~RendererMgr() override;
	};
}