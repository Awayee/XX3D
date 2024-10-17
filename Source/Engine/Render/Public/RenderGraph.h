#pragma once
#include "RHI/Public/RHI.h"
#include "Core/Public/TArray.h"
#include "Core/Public/TUniquePtr.h"
#include "Render/Public/RenderGraphNode.h"

namespace Render {

	struct RenderGraphView;

	// A render graph without resource manager.
	class RenderGraph {
	public:
		static bool ParrallelNodes;
		NON_COPYABLE(RenderGraph);
		NON_MOVEABLE(RenderGraph);
		RenderGraph(RenderGraphView* view=nullptr);
		RGRenderNode* CreateRenderNode(XString&& name);
		RGTransferNode* CreateTransferNode(XString&& name);
		RGBufferNode* CreateBufferNode(RHIBuffer* buffer, XString&& name);
		RGTextureNode* CreateTextureNode(RHITexture* texture, XString&& name);
		RGTextureNode* CopyTextureNode(RGTextureNode* textureNode, XString&& name);
		RGPresentNode* CreatePresentNode();
		void Run(ICmdAllocator* cmdAlloc);
	private:
		TArray<TUniquePtr<RGNode>> m_Nodes;
		TArray<bool> m_NodesSolved;
		RGNodeID m_PresentNodeID;
		RenderGraphView* m_View;
		TArray<RGNodeID> GetPrevPassNodes(RGNode* node);// the last pass node before the node
		RHIFence* GetNodeFence(RGNode* node);
		void RecursivelyRunPrevNodes(RGNode* node, ICmdAllocator* cmdAlloc);
		void RecursivelyRunPrevNodesParallel(RGNode* node, ICmdAllocator* cmdAlloc);
	};

	struct RenderGraphView {
		struct Node {
			ERGNodeType Type;
			XString Name;
			TArray<RGNodeID> PrevNodes;
		};
		RGNodeID LastID{RG_INVALID_NODE};
		TArray<Node> Nodes;
		uint32 UpdateFrame;
	};
}