#include "Render/Public/RenderGraphNode.h"
#include "Render/Public/RenderGraphResource.h"	
namespace Engine {
	RGNodeBase::RGNodeBase() {
	}

	void RGNodeBase::Input(RGResource* res) {
		m_Inputs.PushBack(res);
		res->AddRef();
	}

	void RGNodeBase::Input(RGResource* res, uint32 i) {
		if(!(m_Inputs.Size() > i)) {
			m_Inputs.Resize(i + 1);
		}
		m_Inputs[i] = res;
		res->AddRef();
	}

	void RGNodeBase::Output(RGResource* res) {
		m_Outputs.PushBack(res);
		res->AddRef();
	}

	void RGNodeBase::Output(RGResource* res, uint32 i) {
		if(!(m_Outputs.Size() > i)) {
			m_Outputs.Resize(i + 1);
		}
		m_Outputs[i] = res;
		res->AddRef();
	}

	void RGNode::Execute(RHICommandBuffer* cmd) {
		m_Func(cmd);
	}
}
