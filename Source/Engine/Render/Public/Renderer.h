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
	private:
		struct CmdArray {
			TArray<RHICommandBufferPtr> Cmds;
			TArray<bool> Lifespans;// for gc
			uint32 AllocIndex{ 0 };
			EQueueType QueueType{ EQueueType::Graphics };
			RHICommandBuffer* Get();
			void Reserve(uint32 size);
			void Reset();
			void GC();
		};
	public:
		NON_COPYABLE(CmdPool);
		NON_MOVEABLE(CmdPool);
		CmdPool();
		~CmdPool() override = default;
		RHICommandBuffer* GetCmd(EQueueType queue) override;
		void Reserve(EQueueType queue, uint32 size) override;
		void Reset();
		void GC();
	private:
		TStaticArray<CmdArray, EnumCast(EQueueType::Count)> m_CmdArrays;
	};

	// run rendering process
	class Renderer {
		SINGLETON_INSTANCE(Renderer);
	public:
		void Run();
		void WaitAllFence();
		void OnWindowSizeChanged(uint32 w, uint32 h);
		template <class T> void SetSceneRenderer() { m_SceneRenderer.Reset(new T); }

		const RenderGraphView& GetRenderGraphView()const;
		void RefreshRenderGraphView();
	private:
		TStaticArray<CmdPool, RHI_FRAME_IN_FLIGHT_MAX> m_CmdPools;
		TStaticArray<RHIFencePtr, RHI_FRAME_IN_FLIGHT_MAX> m_Fences;
		USize2D m_CacheWindowSize;
		TUniquePtr<ISceneRenderer> m_SceneRenderer;
		RenderGraphView m_RGView;
		bool m_SizeDirty;
		bool m_RGViewDirty;
		Renderer();
		~Renderer();
		bool SizeValid() const;
	};
}