#include "Render/Public/RenderGraphPass.h"
#include "Render/Public/RenderGraphResource.h"	
namespace Render {
	RGPassNode::RGPassNode() {
	}

	void RGPassNode::SetName(XXString&& name) {
		m_Name = MoveTemp(name);
	}

	void RGPassNode::Input(RGResourceNode* res) {
		m_Inputs.PushBack(res);
		res->AddRef();
	}

	void RGPassNode::Input(RGResourceNode* res, uint32 i) {
		if(!(m_Inputs.Size() > i)) {
			m_Inputs.Resize(i + 1);
		}
		m_Inputs[i] = res;
		res->AddRef();
	}

	void RGPassNode::Output(RGResourceNode* res) {
		m_Outputs.PushBack(res);
		res->AddRef();
	}

	void RGPassNode::Output(RGResourceNode* res, uint32 i) {
		if(!(m_Outputs.Size() > i)) {
			m_Outputs.Resize(i + 1);
		}
		m_Outputs[i] = res;
		res->AddRef();
	}
}
