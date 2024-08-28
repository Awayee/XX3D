#pragma once
#include "RHI/Public/RHI.h"
#include "Core/Public/TUniquePtr.h"
#include "Render/Public/DrawCall.h"
#include "Render/Public/RenderGraph.h"

namespace Render {
	class ISceneRenderer {
	public:
		virtual ~ISceneRenderer() = default;
		virtual void Render(RenderGraph& rg, RGTextureNode* backBufferNode) = 0;
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

	// run rendering process
	class Renderer {
		SINGLETON_INSTANCE(Renderer);
	public:
		void Run();
		void WaitAllFence();
		void OnWindowSizeChanged(uint32 w, uint32 h);
		template <class T> void SetSceneRenderer() { m_SceneRenderer.Reset(new T); }
	private:
		TStaticArray<CmdPool, RHI_FRAME_IN_FLIGHT_MAX> m_CmdPools;
		TStaticArray<RHIFencePtr, RHI_FRAME_IN_FLIGHT_MAX> m_Fences;
		USize2D m_CacheWindowSize;
		TUniquePtr<ISceneRenderer> m_SceneRenderer;
		bool m_SizeDirty;
		Renderer();
		~Renderer();
		bool SizeValid() const;
	};
}