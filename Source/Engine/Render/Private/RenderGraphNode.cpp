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

	void RGRenderNode::ReadColorTarget(RGTextureNode* node, uint32 i, RHITextureSubDesc subRes) {
		ASSERT(i < RHI_COLOR_TARGET_MAX, "Color target index out of range!");
		const uint8 subResIdx = node->RegisterSubRes(subRes);
		m_ColorTargets[i] = { node, subRes, subResIdx, ERTLoadOption::Load };
		node->SetTargetState(EResourceState::ColorTarget);
		RGNode::Connect(node, this);
	}

	void RGRenderNode::ReadDepthTarget(RGTextureNode* node, RHITextureSubDesc subRes) {
		const uint8 subResIdx = node->RegisterSubRes(subRes);
		m_DepthTarget = { node, subRes, subResIdx, ERTLoadOption::Load };
		node->SetTargetState(EResourceState::DepthStencil);
		RGNode::Connect(node, this);
	}

	void RGRenderNode::WriteColorTarget(RGTextureNode* node, uint32 i, RHITextureSubDesc subRes) {
		ASSERT(i < RHI_COLOR_TARGET_MAX, "Color target index out of range!");
		const uint8 subResIdx = node->RegisterSubRes(subRes);
		m_ColorTargets[i] = {node, subRes, subResIdx, ERTLoadOption::Clear};
		RGNode::Connect(this, node);
	}

	void RGRenderNode::WriteDepthTarget(RGTextureNode* node, RHITextureSubDesc subRes) {
		const uint8 subResIdx = node->RegisterSubRes(subRes);
		m_DepthTarget = { node, subRes, subResIdx, ERTLoadOption::Clear };
		RGNode::Connect(this, node);
	}

	void RGRenderNode::SetArea(const Rect& rect) {
		m_RenderArea = rect;
	}

	void RGRenderNode::Run(ICmdAllocator* cmdAlloc) {
		RHICommandBuffer* cmd = cmdAlloc->GetCmd();
		cmd->Reset();
		if (!m_Name.empty()) {
			cmd->BeginDebugLabel(m_Name.c_str(), nullptr);
		}
		// state transition before rendering
		{
			for (uint32 i = 0; i < RHI_COLOR_TARGET_MAX; ++i) {
				if (!m_ColorTargets[i].Node) {
					break;
				}
				m_ColorTargets[i].Node->TransitionToState(cmd, EResourceState::ColorTarget, m_ColorTargets[i].SubResIndex);
			}
			if (m_DepthTarget.Node) {
				m_DepthTarget.Node->TransitionToState(cmd, EResourceState::DepthStencil, m_DepthTarget.SubResIndex);
			}
		}

		// do rendering
		if (m_Task) {
			// setup render pass info
			RHIRenderPassInfo renderPassInfo;
			for (uint32 i = 0; i < RHI_COLOR_TARGET_MAX; ++i) {
				auto& target = m_ColorTargets[i];
				if (!target.Node) {
					break;
				}
				auto& colorTarget = renderPassInfo.ColorTargets[i];
				colorTarget.Target = target.Node->GetRHI();
				colorTarget.MipIndex = target.SubRes.MipIndex;
				colorTarget.ArrayIndex = target.SubRes.ArrayIndex;
				colorTarget.LoadOp = target.LoadOp;
			}
			if (m_DepthTarget.Node) {
				auto& depthStencilTarget = renderPassInfo.DepthStencilTarget;
				depthStencilTarget.Target = m_DepthTarget.Node->GetRHI();
				depthStencilTarget.MipIndex = m_DepthTarget.SubRes.MipIndex;
				depthStencilTarget.ArrayIndex = m_DepthTarget.SubRes.ArrayIndex;
				depthStencilTarget.DepthLoadOp = m_DepthTarget.LoadOp;
				depthStencilTarget.DepthClear = 1.0f;
			}
			renderPassInfo.RenderArea = m_RenderArea;
			// execute render task
			cmd->BeginRendering(renderPassInfo);
			m_Task(cmd);
			cmd->EndRendering();
		}

		// state transition after rendering 
		bool bPresent = false; // check whether pass is for present // TODO use discrete cmd to present?
		{
			for (uint32 i = 0; i < RHI_COLOR_TARGET_MAX; ++i) {
				if (!m_ColorTargets[i].Node) {
					break;
				}
				m_ColorTargets[i].Node->TransitionToTargetState(cmd, m_ColorTargets[i].SubResIndex);
				if(EResourceState::Present == m_ColorTargets[i].Node->GetTargetState()) {
					bPresent = true;
				}
			}
			if (m_DepthTarget.Node) {
				m_DepthTarget.Node->TransitionToTargetState(cmd, m_DepthTarget.SubResIndex);
			}
		}
		if (!m_Name.empty()) {
			cmd->EndDebugLabel();
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

	uint8 RGTextureNode::RegisterSubRes(RHITextureSubDesc subRes) {
		const uint8 index = (uint8)m_SubResStates.Size();
		m_SubResStates.PushBack({ subRes, EResourceState::Unknown });
		return index;
	}

	void RGTextureNode::TransitionToState(RHICommandBuffer* cmd, EResourceState dstState, uint8 subResIdx) {
		if(dstState != EResourceState::Unknown && m_SubResStates[subResIdx].State != dstState) {
			cmd->TransitionTextureState(m_RHI, m_SubResStates[subResIdx].State, dstState, m_SubResStates[subResIdx].SubRes);
			m_SubResStates[subResIdx].State = dstState;
		}
	}

	void RGTextureNode::TransitionToTargetState(RHICommandBuffer* cmd, uint8 subResIdx) {
		TransitionToState(cmd, m_TargetState, subResIdx);
	}
}
