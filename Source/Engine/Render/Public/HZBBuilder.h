#pragma once
#include "RHI/Public/RHI.h"
#include "Render/Public/RenderGraph.h"
#include "Core/Public/TArray.h"

namespace Render {
	class HZBBuilder {
	public:
		HZBBuilder();
		~HZBBuilder() = default;
		RGTextureNode* BuildFurthest(RHITexture* depth, Render::RGTextureNode* depthNode, Render::RenderGraph& rg);
		RHITexture* GetTexture() { return m_HZB; }
	private:
		RHITexturePtr m_HZB;
		union HZBPSOFlags {
			uint32 Val;
			struct {
				bool Furthest : 1;
				bool Closest : 1;
				bool Output2x2 : 1;// if true, output 2x2 pixels per thread, else 1x1
			};
			HZBPSOFlags(): Val(0){}
		};
		TStaticArray<RHIComputePipelineStatePtr, 8> m_HZBPSOStorage;
		RHIComputePipelineStatePtr m_BlitPSO;
		RHIComputePipelineState* GetHZBPSO(HZBPSOFlags flags);
		void CreateBlitPSO();
		void CreateOutputTexture(uint32 dstSize);
	};
}