#pragma once
#include "RHI/Public/RHI.h"
#include "Core/Public/TUniquePtr.h"
#include "Render/Public/DrawCall.h"
#include "Core/Public/TArrayView.h"
#include "Render/Public/RenderGraph.h"

namespace Render {
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
	public:
		ViewportRenderer();
		~ViewportRenderer() = default;
		void SetRenderArea(const Rect& renderArea);
		void Execute(DrawCallContext* drawCallContext);
		void WaitAllFence();
		void WaitCurrentFence();
	private:
		// render resources
		constexpr static ERHIFormat s_NormalFormat = ERHIFormat::B8G8R8A8_UNORM;
		constexpr static ERHIFormat s_AlbedoFormat = ERHIFormat::B8G8R8A8_UNORM;
		constexpr static ERHIFormat s_ColorFormat{ ERHIFormat::R8G8B8A8_UNORM };
		Rect m_RenderArea;
		RHITexturePtr m_GBufferNormal;
		RHITexturePtr m_GBufferAlbedo;
		RHITexturePtr m_Depth;
		TStaticArray<CmdPool, RHI_FRAME_IN_FLIGHT_MAX> m_CmdPools;
		TStaticArray<RHIFencePtr, RHI_FRAME_IN_FLIGHT_MAX> m_Fences;
		RHIGraphicsPipelineStatePtr MeshGBufferPSO;
		RHIGraphicsPipelineStatePtr m_DeferredLightingPSO;
		void CreateTextures();
		bool IsRenderAreaValid() const;
	};
}