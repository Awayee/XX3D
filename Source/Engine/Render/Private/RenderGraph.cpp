#include "Render/Public/RenderGraph.h"
#include "Core/Public/TQueue.h"
#include "Core/Public/Container.h"
#include "System/Public/Timer.h"

namespace Render {

	bool RenderGraph::ParrallelNodes{ false };

	RenderGraph::RenderGraph(RenderGraphView* view) : m_PresentNodeID(RG_INVALID_NODE), m_View(view) {
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

	RGComputeNode* RenderGraph::CreateComputeNode(XString&& name) {
		RGComputeNode* node = (RGComputeNode*)m_Nodes.EmplaceBack(new RGComputeNode(m_Nodes.Size())).Get();
		node->SetName(MoveTemp(name));
		return node;
	}

	RGBufferNode* RenderGraph::CreateBufferNode(RHIBuffer* buffer, XString&& name) {
		if (!buffer->GetName()) {
			buffer->SetName(name.c_str());
		}
		RGBufferNode* node = (RGBufferNode*)m_Nodes.EmplaceBack(new RGBufferNode(m_Nodes.Size(), buffer)).Get();
		node->SetName(MoveTemp(name));
		return node;
	}

	RGTextureNode* RenderGraph::CreateTextureNode(RHITexture* texture, XString&& name) {
		if(!texture->GetName()) {
			texture->SetName(name.c_str());
		}
		RGTextureNode* node = (RGTextureNode*)m_Nodes.EmplaceBack(new RGTextureNode(m_Nodes.Size(), texture)).Get();
		node->SetName(MoveTemp(name));
		return node;
	}

	RGTextureNode* RenderGraph::CopyTextureNode(RGTextureNode* textureNode, XString&& name) {
		RGTextureNode* node = (RGTextureNode*)m_Nodes.EmplaceBack(new RGTextureNode(m_Nodes.Size(), textureNode)).Get();
		node->SetName(MoveTemp(name));
		return node;
	}

	RGOutputNode* RenderGraph::CreateOutputNode(RGTextureNode* prevNode, XString&& name) {
		RGNodeID nodeID = m_Nodes.Size();
		RGOutputNode* node = (RGOutputNode*)m_Nodes.EmplaceBack(new RGOutputNode(nodeID)).Get();
		node->SetName(MoveTemp(name));
		RGNode::Connect(prevNode, node);
		m_Outputs.PushBack(nodeID);
		return node;
	}

	RGPresentNode* RenderGraph::CreatePresentNode(RGTextureNode* prevNode, XString&& name) {
		CHECK(RG_INVALID_NODE == m_PresentNodeID); // num of present node is up to 1
		RGNodeID nodeID = m_Nodes.Size();
		RGPresentNode* node = (RGPresentNode*)m_Nodes.EmplaceBack(new RGPresentNode(nodeID)).Get();
		node->SetName(MoveTemp(name));
		RGNode::Connect(prevNode, node);
		prevNode->SetTargetState(EResourceState::Present);
		m_Outputs.PushBack(nodeID);
		m_PresentNodeID = nodeID;
		return node;
	}

	void RenderGraph::Run(ICmdAllocator* cmdAlloc) {
		ASSERT(RG_INVALID_NODE != m_PresentNodeID, "[RenderGraph::Run] No present node!");
		// run prev nodes and mark
		m_NodesSolved.Resize(m_Nodes.Size(), false);
		// run output nodes
		for(RGNodeID outputID: m_Outputs) {
			RGNode* node = m_Nodes[outputID].Get();
			CHECK(ERGNodeType::Output == node->GetNodeType());
			if(ParrallelNodes) {
				RecursivelyRunPrevNodesParallel(node, cmdAlloc);
			}
			else {
				RecursivelyRunPrevNodes(node, cmdAlloc);
			}
			((RGOutputNode*)node)->Run();
		}

		// record view
		if(m_View) {
			m_View->UpdateFrame = Engine::Timer::GetFrame();
			m_View->OutputIDs = m_Outputs;
			auto& nodeViews = m_View->Nodes;
			nodeViews.Reset();
			nodeViews.Reserve(m_Nodes.Size());
			for(auto& node: m_Nodes) {
				auto& nodeView = nodeViews.EmplaceBack();
				nodeView.Name = node->m_Name;
				nodeView.Type = node->GetNodeType();
				nodeView.PrevNodes = node->m_PrevNodes;
			}
		}
	}

	TArray<RGNodeID> RenderGraph::GetPrevPassNodes(RGNode* node) {
		if(ERGNodeType::Resource == node->GetNodeType()) {
			return node->m_PrevNodes;
		}
		TSet<RGNodeID> nodeIDSet;
		for (const RGNodeID prevID : node->m_PrevNodes) {
			const RGNode* pResNode = m_Nodes[prevID].Get();
			for (const RGNodeID ppID : pResNode->m_PrevNodes) {
				nodeIDSet.insert(ppID);
			}
		}
		TArray<RGNodeID> nodeIDArray; nodeIDArray.Reserve((uint32)nodeIDSet.size());
		for(const RGNodeID ppID: nodeIDSet) {
			nodeIDArray.PushBack(ppID);
		}
		return nodeIDArray;
	}

	RHIFence* RenderGraph::GetNodeFence(RGNode* node) {
		if (ERGNodeType::Pass == node->GetNodeType()) {
			return ((RGPassNode*)node)->m_Fence;
		}
		if (ERGNodeType::Output == node->GetNodeType()) {
			return ((RGPresentNode*)node)->m_Fence;
		}
		return nullptr;
	}

	void RenderGraph::RecursivelyRunPrevNodes(RGNode* node, ICmdAllocator* cmdAlloc) {
		// get fence
		RHIFence* fence = GetNodeFence(node);
		bool isPresent = node->m_NodeID == m_PresentNodeID;
		// execute prev nodes
		const TArray<RGNodeID> prevPassIDs = GetPrevPassNodes(node);
		for(const RGNodeID prevPassID: prevPassIDs) {
			if(!m_NodesSolved[prevPassID]) {
				CHECK(ERGNodeType::Pass == m_Nodes[prevPassID]->GetNodeType());
				RGPassNode* prevPassNode = (RGPassNode*)m_Nodes[prevPassID].Get();
				RecursivelyRunPrevNodes(prevPassNode, cmdAlloc);
				const EQueueType queueType = prevPassNode->GetQueue();
				RHICommandBuffer* cmd = cmdAlloc->GetCmd(queueType);
				prevPassNode->Run(cmd);
				RHI::Instance()->SubmitCommandBuffers(cmd, queueType, fence, isPresent);
				m_NodesSolved[prevPassID] = true;
			}
		}
	}

	void RenderGraph::RecursivelyRunPrevNodesParallel(RGNode* node, ICmdAllocator* cmdAlloc) {
		RHIFence* fence = GetNodeFence(node);
		bool bPresent = node->m_NodeID == m_PresentNodeID;
		// execute prev nodes
		const TArray<RGNodeID> prevPassIDs = GetPrevPassNodes(node);

		//// pre-allocate cmd // TODO because of error in d3d12 when allocating command before last command executed.
		//{
		//	TStaticArray<uint32, EnumCast(EQueueType::Count)> cmdCounts(0);
		//	for(const RGNodeID prevPassID: prevPassIDs) {
		//		if(!m_NodesSolved[prevPassID]) {
		//			CHECK(ERGNodeType::Pass == m_Nodes[prevPassID]->GetNodeType());
		//			RGPassNode* prevPassNode = (RGPassNode*)m_Nodes[prevPassID].Get();
		//			++cmdCounts[EnumCast(prevPassNode->GetQueue())];
		//		}
		//	}
		//	for(uint32 i=0; i<cmdCounts.Size(); ++i) {
		//		cmdAlloc->Reserve((EQueueType)i, cmdCounts[i]);
		//	}
		//}

		TStaticArray<TArray<RHICommandBuffer*>, EnumCast(EQueueType::Count)> cmdArrays;
		for(const RGNodeID prevPassID: prevPassIDs) {
			if(!m_NodesSolved[prevPassID]) {
				CHECK(ERGNodeType::Pass == m_Nodes[prevPassID]->GetNodeType());
				RGPassNode* prevPassNode = (RGPassNode*)m_Nodes[prevPassID].Get();
				RecursivelyRunPrevNodesParallel(prevPassNode, cmdAlloc);
				const EQueueType queue = prevPassNode->GetQueue();
				RHICommandBuffer* cmd = cmdAlloc->GetCmd(queue);
				prevPassNode->Run(cmd);
				cmd->Close();
				cmdArrays[EnumCast(queue)].PushBack(cmd);
				m_NodesSolved[prevPassID] = true;
			}
		}
		// submit cmds of prev nodes with fence
		for (uint32 i = 0; i < EnumCast(EQueueType::Count); ++i) {
			if (auto& cmdArray = cmdArrays[i]; cmdArray.Size()) {
				RHI::Instance()->SubmitCommandBuffers(cmdArray, (EQueueType)i, fence, bPresent);
			}
		}
	}
}
