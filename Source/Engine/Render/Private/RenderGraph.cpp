#include "Render/Public/RenderGraph.h"

namespace Render {
	RenderGraph::RenderGraph() {
		m_Cmd = RHI::Instance()->CreateCommandBuffer();
	}

	void RenderGraph::Execute() {
		// find the start pass (with out input)
		TVector<RGPassNode*> startNodes;
		for(auto& node: m_PassNodes) {
			if(node->m_Inputs.IsEmpty()) {
				startNodes.PushBack(node);
			}
		}
		// TODO
	}
}
