#include "Render/Public/RenderGraphNode.h"

namespace Render {

	void RGNode::Connect(RGNode* prevNode, RGNode* nextNode) {
		++prevNode->m_RefCount;
		++nextNode->m_RefCount;
		prevNode->m_NextNodes.PushBack(nextNode->m_NodeID);
		nextNode->m_PrevNodes.PushBack(prevNode->m_NodeID);
	}

	void RGRenderNode::ReadSRV(RGTextureNode* node) {
		m_SRVs.PushBack(node);
		node->SetTargetState(EResourceState::ShaderResourceView);
		RGNode::Connect(node, this);
	}

	void RGRenderNode::ReadColorTarget(RGTextureNode* node, uint32 i) {
		ASSERT(i < RHI_COLOR_TARGET_MAX, "Color target index out of range!");
		m_ColorTargets[i] = { node, {}, ERTLoadOption::Load };
		node->SetTargetState(EResourceState::ColorTarget);
		RGNode::Connect(node, this);
	}

	void RGRenderNode::ReadDepthTarget(RGTextureNode* node) {
		m_DepthTarget = { node, {}, ERTLoadOption::Load };
		node->SetTargetState(EResourceState::DepthStencil);
		RGNode::Connect(node, this);
	}

	void RGRenderNode::WriteColorTarget(RGTextureNode* node, uint32 i) {
		ASSERT(i < RHI_COLOR_TARGET_MAX, "Color target index out of range!");
		m_ColorTargets[i] = {node, {}, ERTLoadOption::Clear};
		RGNode::Connect(this, node);
	}

	void RGRenderNode::WriteDepthTarget(RGTextureNode* node) {
		m_DepthTarget = { node, {}, ERTLoadOption::Clear };
		RGNode::Connect(this, node);
	}

	void RGRenderNode::SetArea(const Rect& rect) {
		m_RenderArea = rect;
	}

	void RGRenderNode::Run(ICmdAllocator* cmdAlloc) {
		RHICommandBuffer* cmd = cmdAlloc->GetCmd();
		cmd->Reset();
		// state transition before rendering
		{
			for (uint32 i = 0; i < RHI_COLOR_TARGET_MAX; ++i) {
				if (!m_ColorTargets[i].Node) {
					break;
				}
				m_ColorTargets[i].Node->TransitionToState(cmd, EResourceState::ColorTarget);
			}
			if (m_DepthTarget.Node) {
				m_DepthTarget.Node->TransitionToState(cmd, EResourceState::DepthStencil);
			}
		}

		// do rendering
		if (m_Task) {
			// setup render pass info
			RHIRenderPassInfo renderPassInfo;
			for (uint32 i = 0; i < RHI_COLOR_TARGET_MAX; ++i) {
				auto& [node, sub, op] = m_ColorTargets[i];
				if (!node) {
					break;
				}
				auto& colorTarget = renderPassInfo.ColorTargets[i];
				colorTarget.Target = node->GetRHI();
				colorTarget.MipIndex = sub.MipIndex;
				colorTarget.LayerIndex = sub.LayerIndex;
				colorTarget.LoadOp = op;
			}
			if (m_DepthTarget.Node) {
				auto& depthStencilTarget = renderPassInfo.DepthStencilTarget;
				depthStencilTarget.Target = m_DepthTarget.Node->GetRHI();
				depthStencilTarget.DepthLoadOp = m_DepthTarget.LoadOp;
			}
			renderPassInfo.RenderArea = m_RenderArea;
			// execute render task
			if(!m_Name.empty()) {
				cmd->BeginDebugLabel(m_Name.c_str(), nullptr);
			}
			cmd->BeginRendering(renderPassInfo);
			m_Task(cmd);
			cmd->EndRendering();
			if (!m_Name.empty()) {
				cmd->EndDebugLabel();
			}
		}

		// state transition after rendering 
		bool bPresent = false; // check whether pass is for present // TODO use discrete cmd to present?
		{
			for (uint32 i = 0; i < RHI_COLOR_TARGET_MAX; ++i) {
				if (!m_ColorTargets[i].Node) {
					break;
				}
				m_ColorTargets[i].Node->TransitionToTargetState(cmd);
				if(EResourceState::Present == m_ColorTargets[i].Node->GetTargetState()) {
					bPresent = true;
				}
			}
			if (m_DepthTarget.Node) {
				m_DepthTarget.Node->TransitionToTargetState(cmd);
			}
		}
		RHI::Instance()->SubmitCommandBuffer(cmd, m_Fence, bPresent);
	}

	void RGPresentNode::SetPrevNode(RGTextureNode* node) {
		m_PrevNode = node;
		node->SetTargetState(EResourceState::Present);
		RGNode::Connect(node, this);
	}

	void RGPresentNode::Run() {
		if(m_Task) {
			m_Task();
		}
	}

	RHIBuffer* RGBufferNode::GetRHI() {
		if(!m_RHI) {
			m_RHI = RHI::Instance()->CreateBuffer(m_Desc);
		}
		return m_RHI;
	}

	RHITexture* RGTextureNode::GetRHI() {
		if(!m_RHI) {
			m_RHI = RHI::Instance()->CreateTexture(m_Desc);
		}
		return m_RHI;
	}

	void RGTextureNode::TransitionToState(RHICommandBuffer* cmd, EResourceState dstState) {
		if(dstState != EResourceState::Unknown && m_CurrentState != dstState) {
			cmd->TransitionTextureState(m_RHI, m_CurrentState, dstState);
			m_CurrentState = dstState;
		}
	}

	void RGTextureNode::TransitionToTargetState(RHICommandBuffer* cmd) {
		TransitionToState(cmd, m_TargetState);
	}
}
