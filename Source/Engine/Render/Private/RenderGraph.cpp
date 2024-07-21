#include "Render/Public/RenderGraph.h"

namespace Render {
	RenderGraph::RenderGraph() {
		m_Cmd = RHI::Instance()->CreateCommandBuffer();
	}

	RenderGraph::RenderGraph(RenderGraph&& rhs) noexcept: m_Cmd(MoveTemp(rhs.m_Cmd)) {
	}

	RenderGraph& RenderGraph::operator=(RenderGraph&& rhs) noexcept {
		m_Cmd = MoveTemp(rhs.m_Cmd);
		return *this;
	}

	void RenderGraph::Execute() {
		for(auto& node: m_Nodes) {
			node->Execute(m_Cmd.Get());
		}
	}

	void RenderGraph::CreateNodeInternal(TUniquePtr<RGNodeBase>&& node) {
	}
}
