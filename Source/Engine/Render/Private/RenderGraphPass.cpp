#include "Render/Public/RenderGraphPass.h"
#include "Render/Public/RenderGraphResource.h"	
namespace Render {
	RGPassNode::RGPassNode() {
	}

	void RGPassNode::SetName(XString&& name) {
		m_Name = MoveTemp(name);
	}

	void RGPassNode::Input(RGResourceNode* res, uint32 i) {
		if(!(m_Inputs.Size() > i)) {
			m_Inputs.Resize(i + 1);
		}
		m_Inputs[i] = res;
		res->AddRef();
	}

	void RGPassNode::OutputColor(RGResourceNode* node, uint32 i) {
		if (!(m_ColorOutputs.Size() > i)) {
			m_ColorOutputs.Resize(i + 1);
		}
		m_ColorOutputs[i] = node;
		node->AddRef();
	}

	void RGPassNode::OutputDepth(RGResourceNode* node) {
		m_DepthOutput = node;
	}
}
