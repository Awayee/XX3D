#include "Render/Public/RenderGraph.h"

namespace Render {
	RenderGraph::RenderGraph() : m_PresentNodeID(RG_INVALID_NODE) {
	}

	RGRenderNode* RenderGraph::CreateRenderNode(XString&& name) {
		RGRenderNode* node = (RGRenderNode*)m_Nodes.EmplaceBack(new RGRenderNode(m_Nodes.Size())).Get();
		node->SetName(MoveTemp(name));
		return node;
	}

	RGTransferNode* RenderGraph::CreateTransferNode(XString&& name) {
		RGTransferNode* node = (RGTransferNode*)m_Nodes.EmplaceBack(new RGTransferNode(m_Nodes.Size())).Get();
		node->SetName(MoveTemp(name));
		return node;
	}

	RGBufferNode* RenderGraph::CreateBufferNode(RHIBuffer* buffer, XString&& name) {
		RGBufferNode* node = (RGBufferNode*)m_Nodes.EmplaceBack(new RGBufferNode(m_Nodes.Size(), buffer)).Get();
		node->SetName(MoveTemp(name));
		return node;
	}

	RGTextureNode* RenderGraph::CreateTextureNode(RHITexture* texture, XString&& name) {
		RGTextureNode* node = (RGTextureNode*)m_Nodes.EmplaceBack(new RGTextureNode(m_Nodes.Size(), texture)).Get();
		node->SetName(MoveTemp(name));
		return node;
	}

	RGPresentNode* RenderGraph::CreatePresentNode() {
		if(RG_INVALID_NODE == m_PresentNodeID) {
			m_PresentNodeID = m_Nodes.Size();
			m_Nodes.EmplaceBack(new RGPresentNode(m_PresentNodeID))->SetName("Present");
		}
		return (RGPresentNode*)m_Nodes[m_PresentNodeID].Get();
	}

	void RenderGraph::InsertFence(RHIFence* fence, RGNode* node) {
		if(ERGNodeType::Pass == node->GetNodeType()) {
			((RGPassNode*)node)->InsertFence(fence);
		}
		else if(!node->m_PrevNodes.IsEmpty()){
			InsertFence(fence, m_Nodes[node->m_PrevNodes.Back()]);
		}
	}

	void RenderGraph::Run(ICmdAllocator* cmdAlloc) {
		ASSERT(RG_INVALID_NODE != m_PresentNodeID, "[RenderGraph::Run] No present node!");
		m_NodesSolved.Resize(m_Nodes.Size(), false);
		// backtrace the order of pass nodes.
		RecursivelyRunNode(m_PresentNodeID, cmdAlloc);
	}

	void RenderGraph::RecursivelyRunNode(RGNodeID nodeID, ICmdAllocator* cmdAlloc) {
		RGNode* node = m_Nodes[nodeID].Get();
		// wait the dependencies run finished
		for(const RGNodeID prevNodeID: node->m_PrevNodes) {
			RecursivelyRunNode(prevNodeID, cmdAlloc);
		}

		// run node
		if (m_NodesSolved[nodeID]) {
			return;
		}
		if (ERGNodeType::Pass == node->GetNodeType()) {
			((RGPassNode*)node)->Run(cmdAlloc);
		}
		else if(ERGNodeType::Present == node->GetNodeType()) {
			((RGPresentNode*)node)->Run();
		}
		m_NodesSolved[nodeID] = true;
	}
}
