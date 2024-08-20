#pragma once
#include "RHI/Public/RHI.h"
#include "Core/Public/Func.h"
#include "Core/Public/TArray.h"
#include "Core/Public/TUniquePtr.h"
#include "Render/Public/RenderGraphNode.h"

namespace Render {
	class RenderGraph {
	public:
		NON_COPYABLE(RenderGraph);
		NON_MOVEABLE(RenderGraph);
		RenderGraph();
		RGRenderNode* CreateRenderNode(XString&& name);
		RGBufferNode* CreateBufferNode(RHIBuffer* buffer, XString&& name);
		RGTextureNode* CreateTextureNode(RHITexture* texture, XString&& name);
		RGPresentNode* CreatePresentNode();
		void Run(ICmdAllocator* cmdAlloc);
	private:
		TArray<TUniquePtr<RGNode>> m_Nodes;
		TArray<bool> m_NodesSolved;
		RGNodeID m_PresentNodeID;
		void RecursivelyRunNode(RGNodeID nodeID, ICmdAllocator* cmdAlloc);
	};
}