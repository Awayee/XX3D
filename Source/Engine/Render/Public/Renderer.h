#pragma once
#include "RHI/Public/RHI.h"
#include "Core/Public/TUniquePtr.h"
#include "Render/Public/DrawCall.h"
#include "Core/Public/TArrayView.h"
#include "Render/Public/RenderGraph.h"

namespace Render {
	class IRenderScene {
	public:
		virtual RGTextureNode* Render(RenderGraph& rg) = 0;
	};

	class CmdPool: public ICmdAllocator {
	public:
		NON_COPYABLE(CmdPool);
		NON_MOVEABLE(CmdPool);
		CmdPool();
		~CmdPool() override = default;
		RHICommandBuffer* GetCmd() override;
		void Reset();
		void GC();
	private:
		TArray<RHICommandBufferPtr> m_Cmds;
		TArray<bool> m_CmdsLifespan;// for gc
		uint32 m_AllocatedIndex;
	};

	class ViewportRenderer {
		SINGLETON_INSTANCE(ViewportRenderer);
	public:
		void Run();
		void WaitAllFence();
		void OnWindowSizeChanged(uint32 w, uint32 h);
		void SetRenderScene(IRenderScene* scene) { m_RenderScene = scene; }
	private:
		TStaticArray<CmdPool, RHI_FRAME_IN_FLIGHT_MAX> m_CmdPools;
		TStaticArray<RHIFencePtr, RHI_FRAME_IN_FLIGHT_MAX> m_Fences;
		USize2D m_CacheWindowSize;
		IRenderScene* m_RenderScene;
		bool m_SizeDirty;
		ViewportRenderer();
		~ViewportRenderer();
		bool SizeValid() const;
	};
}