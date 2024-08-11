#pragma once
#include "RHI/Public/RHI.h"
#include "Core/Public/Func.h"
#include "Core/Public/String.h"

namespace Render {
	class RGResourceNode;

	class RGPassNode {
	public:
		RGPassNode();
		void SetName(XString&& name);
		void Input(RGResourceNode* res, uint32 i);
		void OutputColor(RGResourceNode* node, uint32 i);
		void OutputDepth(RGResourceNode* node);
		virtual void Execute(RHICommandBuffer* cmd) = 0;
	private:
		XString m_Name;
		friend class RenderGraph;
		TVector<RGResourceNode*> m_Inputs;
		TVector<RGResourceNode*> m_ColorOutputs;
		RGResourceNode* m_DepthOutput;
	};
}