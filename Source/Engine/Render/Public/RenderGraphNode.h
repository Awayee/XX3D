#pragma once
#include "RHI/Public/RHI.h"
#include "Core/Public/Func.h"
#include "Core/Public/String.h"

namespace Engine {
	class RGResource;

	class RGNodeBase {
	private:
		XXString m_Name;
		friend class RenderGraph;
		TVector<RGResource*> m_Inputs;
		TVector<RGResource*> m_Outputs;
	public:
		RGNodeBase();
		void Input(RGResource* res);
		void Input(RGResource* res, uint32 i);
		void Output(RGResource* res);
		void Output(RGResource* res, uint32 i);
		virtual void Execute(RHICommandBuffer* cmd) = 0;
	};

	class RGNode: public RGNodeBase {
	public:
		void Execute(RHICommandBuffer* cmd) override;
		typedef Func<void(RHICommandBuffer*)> NodeFunc;
	private:
		NodeFunc m_Func;
	};
}