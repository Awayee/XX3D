#include "Render/Public/RenderGraphNode.h"
#include "Math/Public/Math.h"

namespace Render {

	void NodeCmdCollector::AddCommand(EQueueType queue, RHICommandBuffer* cmd) {
		m_CmdArrays[EnumCast(queue)].PushBack(cmd);
	}

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

	void RGRenderNode::ReadColorTarget(RGTextureNode* node, uint32 i, RHITextureSubRes subRes, bool bClear) {
		ASSERT(i < RHI_COLOR_TARGET_MAX, "Color target index out of range!");
		const uint8 subResIdx = node->RegisterSubRes(subRes);
		m_ColorTargets[i] = { node, subRes, subResIdx, bClear};
		node->SetTargetState(EResourceState::ColorTarget);
		RGNode::Connect(node, this);
	}

	void RGRenderNode::ReadColorTarget(RGTextureNode* node, uint32 i, bool bClear) {
		ReadColorTarget(node, i, node->GetDesc().GetDefaultSubRes(), bClear);
	}

	void RGRenderNode::WriteColorTarget(RGTextureNode* node, uint32 i, RHITextureSubRes subRes, bool bClear) {
		ASSERT(i < RHI_COLOR_TARGET_MAX, "Color target index out of range!");
		const uint8 subResIdx = node->RegisterSubRes(subRes);
		m_ColorTargets[i] = {node, subRes, subResIdx, bClear};

		const auto& backBufferDesc = node->GetDesc();
		m_RenderArea = { 0,0, backBufferDesc.Width, backBufferDesc.Height };
		RGNode::Connect(this, node);
	}

	void RGRenderNode::WriteColorTarget(RGTextureNode* node, uint32 i, bool bClear) {
		WriteColorTarget(node, i, node->GetDesc().GetDefaultSubRes(), bClear);
	}

	void RGRenderNode::ReadDepthTarget(RGTextureNode* node, RHITextureSubRes subRes, bool bClear) {
		const uint8 subResIdx = node->RegisterSubRes(subRes);
		m_DepthTarget = { node, subRes, subResIdx, bClear };
		node->SetTargetState(EResourceState::DepthStencil);
		RGNode::Connect(node, this);
	}

	void RGRenderNode::ReadDepthTarget(RGTextureNode* node, bool bClear) {
		ReadDepthTarget(node, node->GetDesc().GetDefaultSubRes(), bClear);
	}

	void RGRenderNode::WriteDepthTarget(RGTextureNode* node, RHITextureSubRes subRes, bool bClear) {
		const uint8 subResIdx = node->RegisterSubRes(subRes);
		m_DepthTarget = { node, subRes, subResIdx, bClear };
		RGNode::Connect(this, node);
	}

	void RGRenderNode::WriteDepthTarget(RGTextureNode* node, bool bClear) {
		WriteDepthTarget(node, node->GetDesc().GetDefaultSubRes(), bClear);
	}

	void RGRenderNode::Run(RHICommandBuffer* cmd) {
		if (!m_Name.empty()) {
			cmd->BeginDebugLabel(m_Name.c_str(), nullptr);
		}
		// state transition before rendering
		{
			for (uint32 i = 0; i < RHI_COLOR_TARGET_MAX; ++i) {
				if (!m_ColorTargets[i].Node) {
					break;
				}
				m_ColorTargets[i].Node->TransitionSubResToState(cmd, EResourceState::ColorTarget, m_ColorTargets[i].SubResIndex);
			}
			if (m_DepthTarget.Node) {
				m_DepthTarget.Node->TransitionSubResToState(cmd, EResourceState::DepthStencil, m_DepthTarget.SubResIndex);
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
				colorTarget.LoadOp = target.IsClear ? ERTLoadOption::Clear : ERTLoadOption::Load;
			}
			if (m_DepthTarget.Node) {
				auto& depthStencilTarget = renderPassInfo.DepthStencilTarget;
				depthStencilTarget.Target = m_DepthTarget.Node->GetRHI();
				depthStencilTarget.MipIndex = m_DepthTarget.SubRes.MipIndex;
				depthStencilTarget.ArrayIndex = m_DepthTarget.SubRes.ArrayIndex;
				depthStencilTarget.DepthLoadOp = m_DepthTarget.IsClear ? ERTLoadOption::Clear : ERTLoadOption::Load;
				depthStencilTarget.DepthClear = 1.0f;
			}
			renderPassInfo.RenderArea = m_RenderArea;
			// execute render task
			cmd->BeginRendering(renderPassInfo);
			m_Task(cmd);
			cmd->EndRendering();
		}

		// state transition after rendering 
		{
			for (uint32 i = 0; i < RHI_COLOR_TARGET_MAX; ++i) {
				if (!m_ColorTargets[i].Node) {
					break;
				}
				m_ColorTargets[i].Node->TransitionSubResToTargetState(cmd, m_ColorTargets[i].SubResIndex);
			}
			if (m_DepthTarget.Node) {
				m_DepthTarget.Node->TransitionSubResToTargetState(cmd, m_DepthTarget.SubResIndex);
			}
		}
		if (!m_Name.empty()) {
			cmd->EndDebugLabel();
		}
	}

	void RGComputeNode::ReadSRV(RGTextureNode* node, RHITextureSubRes subRes) {
		const uint8 subResIdx = node->RegisterSubRes(subRes);
		m_SRVs.PushBack({node, subResIdx});
		node->SetSubResTargetState(subResIdx, EResourceState::ShaderResourceView);
		RGNode::Connect(node, this);
	}

	void RGComputeNode::ReadSRV(RGTextureNode* node) {
		ReadSRV(node, node->GetDesc().GetDefaultSubRes());
	}

	void RGComputeNode::WriteUAV(RGTextureNode* node, RHITextureSubRes subRes) {
		const uint8 subResIdx = node->RegisterSubRes(subRes);
		m_UAVs.PushBack({ node, subResIdx });
		node->SetSubResTargetState(subResIdx, EResourceState::ShaderResourceView);
		RGNode::Connect(this, node);
	}

	void RGComputeNode::WriteUAV(RGTextureNode* node) {
		WriteUAV(node, node->GetDesc().GetDefaultSubRes());
	}

	void RGComputeNode::Run(RHICommandBuffer* cmd) {
		if (!m_Name.empty()) {
			cmd->BeginDebugLabel(m_Name.c_str(), nullptr);
		}
		// state transition before dispatch
		{
			for (auto& srv : m_SRVs) {
				srv.Node->TransitionSubResToState(cmd, EResourceState::ShaderResourceView, srv.SubResIndex);
			}
			for (auto& uav : m_UAVs) {
				uav.Node->TransitionSubResToState(cmd, EResourceState::UnorderedAccessView, uav.SubResIndex);
			}
		}
		if (m_Task) {
			m_Task(cmd);
		}
		// state transition after dispatch
		{
			for (auto& srv : m_SRVs) {
				srv.Node->TransitionSubResToTargetState(cmd, srv.SubResIndex);
			}
			for (auto& uav : m_UAVs) {
				uav.Node->TransitionSubResToTargetState(cmd, uav.SubResIndex);
			}
		}
		if (!m_Name.empty()) {
			cmd->EndDebugLabel();
		}
	}

	void RGTransferNode::CopyTexture(RGTextureNode* src, RGTextureNode* dst, RHITextureSubRes subRes) {
		auto& cpyPair = m_Cpy.EmplaceBack();
		cpyPair.Src = src;
		cpyPair.Dst = dst;
		cpyPair.SubRes = subRes;
		uint32 w = Math::Min<uint32>(src->GetDesc().Width, dst->GetDesc().Width);
		uint32 h = Math::Min<uint32>(src->GetDesc().Height, dst->GetDesc().Height);
		cpyPair.CpySize = {w, h};
		cpyPair.SrcSubResIndex = src->RegisterSubRes(subRes);
		cpyPair.DstSubResIndex = dst->RegisterSubRes(subRes);
		RGNode::Connect(src, this);
		RGNode::Connect(this, dst);
	}

	void RGTransferNode::CopyTexture(RGTextureNode* src, RGTextureNode* dst) {
		CopyTexture(src, dst, src->GetDesc().GetDefaultSubRes());
	}

	void RGTransferNode::Run(RHICommandBuffer* cmd) {
		if(!m_Name.empty()) {
			cmd->BeginDebugLabel(m_Name.c_str(), nullptr);
		}
		for(auto& cpyPair: m_Cpy) {
			// transition state
			cpyPair.Src->TransitionSubResToState(cmd, EResourceState::TransferSrc, cpyPair.SrcSubResIndex);
			cpyPair.Dst->TransitionSubResToState(cmd, EResourceState::TransferDst, cpyPair.DstSubResIndex);
			RHITextureCopyRegion region{};
			region.DstSubRes = region.SrcSubRes = cpyPair.SubRes;
			region.SrcOffset = region.DstOffset = { 0, 0, 0 };
			region.Extent = { cpyPair.CpySize.w, cpyPair.CpySize.h, 1 };
			cmd->CopyTextureToTexture(cpyPair.Src->GetRHI(), cpyPair.Dst->GetRHI(), region);
			cpyPair.Src->TransitionSubResToTargetState(cmd, cpyPair.SrcSubResIndex);
			cpyPair.Dst->TransitionSubResToTargetState(cmd, cpyPair.DstSubResIndex);
		}
		if (!m_Name.empty()) {
			cmd->EndDebugLabel();
		}
	}

	void RGPresentNode::Run() {
		RHI::Instance()->GetViewport()->Present();
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

	void RGTextureNode::SetTargetState(EResourceState targetState) {
		m_TargetState = targetState;
	}

	void RGTextureNode::SetSubResTargetState(uint8 subResIdx, EResourceState targetState) {
		if(DEFAULT_SUB_RES == subResIdx) {
			SetTargetState(targetState);
			return;
		}
		m_SubResStates[subResIdx].TargetState = targetState;
	}

	uint8 RGTextureNode::RegisterSubRes(RHITextureSubRes subRes) {
		if(subRes == m_Desc.GetDefaultSubRes()) {
			return DEFAULT_SUB_RES;
		}
		for(uint32 i=0; i< m_SubResStates.Size(); ++i) {
			if(m_SubResStates[i].SubRes == subRes) {
				return (uint8)i;
			}
		}
		const uint8 index = (uint8)m_SubResStates.Size();
		m_SubResStates.PushBack({ subRes, m_TargetState });
		return index;
	}

	void RGTextureNode::TransitionSubResToState(RHICommandBuffer* cmd, EResourceState dstState, uint8 subResIdx) {
		if(DEFAULT_SUB_RES == subResIdx) {
			TransitionToState(cmd, dstState);
			return;
		}
		if(dstState != EResourceState::Unknown && m_SubResStates[subResIdx].CurrentState != dstState) {
			cmd->TransitionTextureState(m_RHI, m_SubResStates[subResIdx].CurrentState, dstState, m_SubResStates[subResIdx].SubRes);
			m_SubResStates[subResIdx].CurrentState = dstState;
		}
	}

	void RGTextureNode::TransitionSubResToTargetState(RHICommandBuffer* cmd, uint8 subResIdx) {
		if(DEFAULT_SUB_RES == subResIdx) {
			TransitionToTargetState(cmd);
			return;
		}
		auto& subResState = m_SubResStates[subResIdx];
		const EResourceState dstState = subResState.TargetState != EResourceState::Unknown ? subResState.TargetState : m_TargetState;
		if(EResourceState::Unknown != dstState && subResState.CurrentState != dstState) {
			cmd->TransitionTextureState(m_RHI, subResState.CurrentState, dstState, subResState.SubRes);
			subResState.CurrentState = dstState;
		}
	}

	void RGTextureNode::TransitionToState(RHICommandBuffer* cmd, EResourceState dstState) {
		if(dstState != EResourceState::Unknown && m_CurrentState != dstState) {
			cmd->TransitionTextureState(m_RHI, m_CurrentState, dstState, m_Desc.GetDefaultSubRes());
			for(auto& subResState: m_SubResStates) {
				subResState.CurrentState = dstState;
			}
			m_CurrentState = dstState;
		}
	}

	void RGTextureNode::TransitionToTargetState(RHICommandBuffer* cmd) {
		TransitionToState(cmd, m_TargetState);
	}
}
