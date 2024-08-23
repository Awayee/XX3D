#pragma once
#include "RHI/Public/RHI.h"
#include "Core/Public/Defines.h"
#include "Core/Public/Func.h"
#include "Core/Public/String.h"

namespace Render {
	class RGResourceNode;
	class RGTextureNode;
	class RGBufferNode;
	class RenderGraph;
	enum class ERGNodeType {
		Pass,
		Resource,
		Present
	};
	typedef uint32 RGNodeID;
	constexpr RGNodeID RG_INVALID_NODE = UINT32_MAX;


	class ICmdAllocator {
	public:
		virtual ~ICmdAllocator() = default;
		virtual RHICommandBuffer* GetCmd() = 0;
	};

	class RGNode {
	public:
		RGNode(RGNodeID nodeID) : m_NodeID(nodeID), m_RefCount(0) {}
		virtual ~RGNode() = default;
		void SetName(XString&& name) { m_Name = MoveTemp(name); }
	protected:
		friend RenderGraph;
		RGNodeID m_NodeID;
		XString m_Name;
		uint32 m_RefCount;
		TArray<RGNodeID> m_PrevNodes;
		TArray<RGNodeID> m_NextNodes;
		virtual ERGNodeType GetNodeType() const = 0;
		static void Connect(RGNode* prevNode, RGNode* nextNode);
	};

#pragma region PassNode
	class RGPassNode: public RGNode {
	public:
		RGPassNode(RGNodeID nodeID) : RGNode(nodeID) {}
	protected:
		friend RenderGraph;
		ERGNodeType GetNodeType() const override { return ERGNodeType::Pass; }
		virtual void Run(ICmdAllocator* cmdAlloc) = 0;
	};

	class RGRenderNode: public RGPassNode {
	public:
		typedef Func<void(RHICommandBuffer*)> RenderTask;
		RGRenderNode(uint32 nodeID): RGPassNode(nodeID), m_Fence(nullptr) {}
		~RGRenderNode() override = default;
		void ReadSRV(RGTextureNode* node);
		void ReadColorTarget(RGTextureNode* node, uint32 i, RHITextureSubDesc subRes={}); // keep content written by previous pass (without clear)
		void ReadDepthTarget(RGTextureNode* node);
		void WriteColorTarget(RGTextureNode* node, uint32 i, RHITextureSubDesc subRes={}); // write new targets
		void WriteDepthTarget(RGTextureNode* node);
		void SetArea(const Rect& rect);
		void SetTask(RenderTask&& f) { m_Task = MoveTemp(f); }
		void InsertFence(RHIFence* fence) { m_Fence = fence; }
	private:
		TArray<RGTextureNode*> m_SRVs;
		struct TargetInfo {
			RGTextureNode* Node{ nullptr };
			RHITextureSubDesc SubRes;
			ERTLoadOption LoadOp{ ERTLoadOption::NoAction };
		};
		TargetInfo m_DepthTarget;
		TStaticArray<TargetInfo, RHI_COLOR_TARGET_MAX> m_ColorTargets;
		Rect m_RenderArea;
		RenderTask m_Task;
		RHIFence* m_Fence;
		void Run(ICmdAllocator* cmdAlloc) override;
	};

	class RGPresentNode: public RGNode {
	public:
		typedef Func<void()> PresentTask;
		RGPresentNode(RGNodeID nodeID) : RGNode(nodeID), m_PrevNode(nullptr){}
		~RGPresentNode() override = default;
		void SetPrevNode(RGTextureNode* node);
		void SetTask(PresentTask&& task) { m_Task = MoveTemp(task); }
	private:
		friend RenderGraph;
		RGTextureNode* m_PrevNode;
		PresentTask m_Task;
		ERGNodeType GetNodeType() const override { return ERGNodeType::Present; }
		void Run();
	};

#pragma endregion

#pragma region ResourceNode

	class RGResourceNode : public RGNode{
	public:
		RGResourceNode(RGNodeID nodeID) : RGNode(nodeID){}
	private:
		ERGNodeType GetNodeType() const override { return ERGNodeType::Resource; }
	};

	class RGBufferNode : public RGResourceNode {
	public:
		RGBufferNode(RGNodeID nodeID, const RHIBufferDesc& desc) : RGResourceNode(nodeID), m_Desc(desc), m_RHI(nullptr) {}
		explicit RGBufferNode(uint32 nodeID, RHIBuffer* buffer) : RGResourceNode(nodeID), m_Desc(buffer->GetDesc()), m_RHI(buffer) {}
		RHIBuffer* GetRHI();
	private:
		RHIBufferDesc m_Desc;
		RHIBuffer* m_RHI;
	};

	class RGTextureNode : public RGResourceNode {
	public:
		RGTextureNode(RGNodeID nodeID, const RHITextureDesc& desc) : RGResourceNode(nodeID), m_Desc(desc), m_RHI(nullptr) {}
		explicit RGTextureNode(uint32 nodeID, RHITexture* texture) : RGResourceNode(nodeID), m_Desc(texture->GetDesc()), m_RHI(texture) {}
		RHITexture* GetRHI();
		void SetTargetState(EResourceState targetState) { m_TargetState = targetState; }
		EResourceState GetTargetState() const { return m_CurrentState; }
		void TransitionToState(RHICommandBuffer* cmd, EResourceState dstState, RHITextureSubDesc subRes);
		void TransitionToTargetState(RHICommandBuffer* cmd, RHITextureSubDesc subRes);
	private:
		RHITextureDesc m_Desc;
		RHITexture* m_RHI;
		EResourceState m_CurrentState{ EResourceState::Unknown }; // the state at pass output
		EResourceState m_TargetState{ EResourceState::Unknown }; // the state at next pass input
	};
#pragma endregion
}