#pragma once
#include "RHI/Public/RHI.h"
#include "Core/Public/Func.h"
#include "Core/Public/String.h"

namespace Render {
	class RGResourceNode;

	class RGPassNode {
	public:
		RGPassNode();
		void SetName(XXString&& name);
		void Input(RGResourceNode* res);
		void Input(RGResourceNode* res, uint32 i);
		void Output(RGResourceNode* res);
		void Output(RGResourceNode* res, uint32 i);
		virtual void Execute(RHICommandBuffer* cmd) = 0;
	private:
		XXString m_Name;
		friend class RenderGraph;
		TVector<RGResourceNode*> m_Inputs;
		TVector<RGResourceNode*> m_Outputs;
	};
}