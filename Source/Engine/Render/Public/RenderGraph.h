#pragma once
#include "RHI/Public/RHI.h"
#include "Core/Public/Func.h"
#include "Core/Public/TVector.h"
#include "Core/Public/TUNiquePtr.h"
#include "Render/Public/RenderGraphPass.h"
#include "Render/Public/RenderGraphResource.h"

namespace Render {
	class RenderGraph {
	public:
		NON_COPYABLE(RenderGraph);
		RenderGraph();
		RenderGraph(RenderGraph&& rhs) noexcept;
		RenderGraph& operator=(RenderGraph&& rhs) noexcept;

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
		TVector<TUniquePtr<RGPassNode>> m_PassNodes;
		TVector<TUniquePtr<RGResourceNode>> m_ResourceNodes;
	};
}