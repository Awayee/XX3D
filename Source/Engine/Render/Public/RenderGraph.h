#pragma once
#include "RHI/Public/RHI.h"
#include "Core/Public/Func.h"
#include "Core/Public/TVector.h"
#include "Core/Public/TUNiquePtr.h"
#include "Render/Public/RenderGraphNode.h"
#include "Render/Public/RenderGraphResource.h"

namespace Render {
	class RenderGraph {
	public:
		RenderGraph();
		RenderGraph(const RenderGraph&) = delete;
		RenderGraph(RenderGraph&& rhs) noexcept;
		RenderGraph& operator=(const RenderGraph&) = delete;
		RenderGraph& operator=(RenderGraph&& rhs) noexcept;

		typedef Func<void(RHICommandBuffer*)> NodeFunc;

		template<class T, class...Args>
		T* CreateNode(Args&&...args) {
			TUniquePtr<T> nodePtr(new T(MoveTemp(args)...));
			T* node = nodePtr.Get();
			RGNodeBase* nodeBase = static_cast<RGNodeBase>(node);
			ASSERT(nodeBase, "");
			CreateNodeInternal(MoveTemp(nodePtr));
			return node;
		}

		template<class T, class...Args>
		T* AllocateParameter(Args&&...args) {
			return nullptr;
		}

		void Execute();
	private:
		RHICommandBufferPtr m_Cmd;
		TVector<TUniquePtr<RGNodeBase>> m_Nodes;
		TVector<TUniquePtr<RGResource>> m_Resources;
		void CreateNodeInternal(TUniquePtr<RGNodeBase>&& node);
	};
}