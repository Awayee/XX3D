#pragma once
#include "RHI/Public/RHI.h"
#include "Core/Public/TUniquePtr.h"
#include "Render/Public/DrawCall.h"
#include "Core/Public/TArrayView.h"

namespace Render {
	class BasePass {
	public:
		NON_COPYABLE(BasePass);
		NON_MOVEABLE(BasePass);
		BasePass();
		~BasePass();
		void SetTargetSize(USize2D size);
		void SetRenderArea(IURect area);
		const IURect& GetRenderArea() const { return m_RenderArea; }
		RHITexture* GetColorTarget(uint32 i);
		RHITexture* GetDepthTarget();
		void Execute(DrawCallQueue& queue);
	private:
		static TStaticArray<ERHIFormat, 2> s_ColorFormats;
		RHIGraphicsPipelineStatePtr m_PSO;
		RHICommandBufferPtr m_Cmd;
		TStaticArray<RHITexturePtr, 2> m_ColorTargets;
		RHITexturePtr m_DepthTarget;
		USize2D m_TargetSize;
		IURect m_RenderArea;
		void ResetTargets();
	};

	class DeferredLightingPass {
	public:
		NON_COPYABLE(DeferredLightingPass);
		NON_MOVEABLE(DeferredLightingPass);
		DeferredLightingPass();
		~DeferredLightingPass();
		void SetInput(uint32 i, RHITexture* input);
		void SetRenderArea(IURect area);
		void Execute();
		RHITexture* GetColorTarget();
	private:
		static ERHIFormat s_ColorFormat;
		RHIGraphicsPipelineStatePtr m_PSO;
		RHICommandBufferPtr m_Cmd;
		TStaticArray<RHITexture*, 4> m_InputTargets;
		RHITexturePtr m_ColorTarget;
		IURect m_RenderArea;
	};

	// The pass directly render for Swapchain
	class PresentPass {
	public:
		NON_COPYABLE(PresentPass);
		NON_MOVEABLE(PresentPass);
		PresentPass();
		~PresentPass();
		void SetDepthTarget(RHITexture* depthTarget);
		void Execute(DrawCallQueue& queue);
		void WaitAll();
		void WaitCurrent();
	private:
		struct FrameResource {
			RHICommandBufferPtr Cmd;
			RHIFencePtr Fence;// rendering complete in this frame.
		};
		TStaticArray<RHICommandBufferPtr, RHI_MAX_FRAME_IN_FLIGHT> m_Cmds;
		TStaticArray<RHIFencePtr, RHI_MAX_FRAME_IN_FLIGHT> m_Fences;
		RHITexture* m_DepthTarget;
	};

	class RendererMgr {
		SINGLETON_INSTANCE(RendererMgr);
	public:
		void Update();
		void WaitQueue();
	private:
		// The last renderer
		BasePass m_BasePass;
		DeferredLightingPass m_DeferredLightingPass;
		PresentPass m_PresentRenderer;
		RendererMgr();
		~RendererMgr();
	};
}