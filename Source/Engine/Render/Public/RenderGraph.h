#pragma once
#include "RHI/Public/RHI.h"
#include "Core/Public/Func.h"
#include "Core/Public/TVector.h"
#include "Core/Public/TUNiquePtr.h"
#include "Render/Public/RenderGraphNode.h"
#include "Render/Public/RenderGraphResource.h"

namespace Engine {
	class RenderGraph {
	public:
		RenderGraph(RHICommandBuffer* cmd);
		RenderGraph(const RenderGraph&) = delete;
		RenderGraph& operator=(const RenderGraph&) = delete;

		typedef Func<void(RHICommandBuffer*)> NodeFunc;

		template<class T, class...Args>
		T* CreateNode(XXString&& name, Args&&...args) {
			TUniquePtr<T> nodePtr = MakeUniquePtr<T>(std::forward<Args>(args)...);
			T* node = nodePtr.Get();
			RGNodeBase* nodeBase = static_cast<RGNodeBase>(node);
			ASSERT(nodeBase, "");
			nodeBase->m_Name = std::move(name);
			CreateNodeInternal(std::move(nodePtr));
			return node;
		}

		template<class T, class...Args>
		T* AllocateParameter(Args&&...args) {
			return nullptr;
		}

		void Execute();
	private:
		RHICommandBuffer* m_Cmd;
		TVector<TUniquePtr<RGNodeBase>> m_Nodes;
		TVector<TUniquePtr<RGResource>> m_Resources;
		void CreateNodeInternal(TUniquePtr<RGNodeBase>&& node);
	};
}