#include "Render/Public/RenderGraph.h"

namespace Engine {
	RenderGraph::RenderGraph(RHICommandBuffer* cmd): m_Cmd(cmd) {
	}

	void RenderGraph::Execute() {
		for(auto& node: m_Nodes) {
			node->Execute(m_Cmd);
		}
	}

	void RenderGraph::CreateNodeInternal(TUniquePtr<RGNodeBase>&& node) {
	}
}
