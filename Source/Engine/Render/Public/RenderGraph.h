#pragma once
#include "RHI/Public/RHI.h"
#include "Core/Public/Func.h"
#include "Core/Public/TArray.h"
#include "Core/Public/TUniquePtr.h"
#include "Render/Public/RenderGraphPass.h"
#include "Render/Public/RenderGraphResource.h"

namespace Render {
	class RenderGraph {
	public:
		NON_COPYABLE(RenderGraph);
		NON_MOVEABLE(RenderGraph);
		RenderGraph();
		typedef Func<void(RHICommandBuffer*)> NodeFunc;
		template<class T> T* CreatePassNode() {
			return m_PassNodes.EmplaceBack(new T()).Get();
		}

		template<class T, class Desc> T* CreateResourceNode(const Desc& desc) {
			return m_ResourceNodes.EmplaceBack(new T(desc)).Get();
		}

		void Execute();
	private:
		RHICommandBufferPtr m_Cmd;
		TArray<TUniquePtr<RGPassNode>> m_PassNodes;
		TArray<TUniquePtr<RGResourceNode>> m_ResourceNodes;
	};
}