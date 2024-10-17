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
		virtual RHICommandBuffer* GetCmd(EQueueType queue) = 0;
		virtual void Reserve(EQueueType queue, uint32 size) = 0;
	};

	class RGNode {
	public:
		RGNode(RGNodeID nodeID) : m_NodeID(nodeID), m_RefCount(0) {}
		virtual ~RGNode() = default;
		void SetName(XString&& name) { m_Name = MoveTemp(name);  }
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
		RGPassNode(RGNodeID nodeID) : RGNode(nodeID), m_Fence(nullptr), m_EnableParallel(false){}
		void InsertFenceBefore(RHIFence* fence) { m_Fence = fence; }
		void EnableParallel() { m_EnableParallel = true; }
	protected:
		friend RenderGraph;
		RHIFence* m_Fence;
		bool m_EnableParallel;
		ERGNodeType GetNodeType() const override { return ERGNodeType::Pass; }
		virtual EQueueType GetQueue() const = 0;
		virtual void Run(RHICommandBuffer* cmd) = 0;
	};

	class RGRenderNode: public RGPassNode {
	public:
		typedef Func<void(RHICommandBuffer*)> RenderTask;
		RGRenderNode(uint32 nodeID): RGPassNode(nodeID) {}
		~RGRenderNode() override = default;
		void ReadSRV(RGTextureNode* node);
		void ReadColorTarget(RGTextureNode* node, uint32 i, RHITextureSubRes subRes, bool bClear=false); // keep content written by previous pass (without clear)
		void ReadColorTarget(RGTextureNode* node, uint32 i, bool bClear=false); // default subres
		void WriteColorTarget(RGTextureNode* node, uint32 i, RHITextureSubRes subRes, bool bClear=true); // write new targets
		void WriteColorTarget(RGTextureNode* node, uint32 i, bool bClear=true);
		void ReadDepthTarget(RGTextureNode* node, RHITextureSubRes subRes, bool bClear=false);
		void ReadDepthTarget(RGTextureNode* node, bool bClear=false);
		void WriteDepthTarget(RGTextureNode* node, RHITextureSubRes subRes, bool bClear=true);
		void WriteDepthTarget(RGTextureNode* node, bool bClear=true);
		void SetRenderArea(const Rect& rect) { m_RenderArea = rect; }
		const Rect& GetRenderArea() const { return m_RenderArea; }
		void SetTask(RenderTask&& f) { m_Task = MoveTemp(f); }
	private:
		TArray<RGTextureNode*> m_SRVs;
		struct TargetInfo {
			RGTextureNode* Node{ nullptr };
			RHITextureSubRes SubRes{};
			uint8 SubResIndex{ 0 };
			bool IsClear{ false };
		};
		TargetInfo m_DepthTarget;
		TStaticArray<TargetInfo, RHI_COLOR_TARGET_MAX> m_ColorTargets;
		Rect m_RenderArea;
		RenderTask m_Task;
		EQueueType GetQueue() const override { return EQueueType::Graphics; }
		void Run(RHICommandBuffer* cmd) override;
	};

	class RGComputeNode: public RGPassNode {
	public:
		typedef Func<void(RHICommandBuffer*)> ComputeTask;
		RGComputeNode(uint32 nodeID) :RGPassNode(nodeID) {}
		~RGComputeNode() override = default;
		void ReadSRV(RGTextureNode* node);
		void WriteUAV(RGTextureNode* node);
		void SetTask(ComputeTask&& f) { m_Task = MoveTemp(f); }
	private:
		TArray<RGTextureNode*> m_SRVs;
		TArray<RGTextureNode*> m_UAVs;
		ComputeTask m_Task;
		EQueueType GetQueue() const override { return EQueueType::Compute; }
		void Run(RHICommandBuffer* cmd) override;
	};

	class RGTransferNode: public RGPassNode {
	public:
		RGTransferNode(uint32 nodeID): RGPassNode(nodeID){}
		void CopyTexture(RGTextureNode* src, RGTextureNode* dst, RHITextureSubRes subRes);
		void CopyTexture(RGTextureNode* src, RGTextureNode* dst);
	private:
		struct CpyResourcePair {
			RGTextureNode* Src;
			RGTextureNode* Dst;
			USize2D CpySize;
			RHITextureSubRes SubRes;
			uint8 SrcSubResIndex;
			uint8 DstSubResIndex;
		};
		TArray<CpyResourcePair> m_Cpy;
		EQueueType GetQueue() const override { return EQueueType::Graphics; } // TODO Transfer queue is preferred, but transition state must in graphics queue
		void Run(RHICommandBuffer* cmd) override;
	};

	class RGPresentNode: public RGNode {
	public:
		typedef Func<void()> PresentTask;
		RGPresentNode(RGNodeID nodeID) : RGNode(nodeID), m_Fence(nullptr) {}
		~RGPresentNode() override = default;
		void Present(RGTextureNode* node);
		void SetTask(PresentTask&& task) { m_Task = MoveTemp(task); }
		void InsertFenceBefore(RHIFence* fence) { m_Fence = fence; }
	private:
		friend RenderGraph;
		RHIFence* m_Fence;
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
		explicit RGTextureNode(uint32 nodeID, const RGTextureNode* rhs) : RGResourceNode(nodeID), m_Desc(rhs->m_Desc), m_RHI(rhs->m_RHI) {}
		const RHITextureDesc& GetDesc() const { return m_Desc; }
		RHITexture* GetRHI();
		void SetTargetState(EResourceState targetState) { m_TargetState = targetState; }
		EResourceState GetTargetState() const { return m_TargetState; }
		uint8 RegisterSubRes(RHITextureSubRes subRes); // register sub resource for connected node, return the index of m_SubResStates
		void TransitionToState(RHICommandBuffer* cmd, EResourceState dstState, uint8 subResIdx);
		void TransitionToState(RHICommandBuffer* cmd, EResourceState dstState);
		void TransitionToTargetState(RHICommandBuffer* cmd, uint8 subResIdx);
		void TransitionToTargetState(RHICommandBuffer* cmd);
	private:
		RHITextureDesc m_Desc;
		RHITexture* m_RHI;
		struct SubResState {
			RHITextureSubRes SubRes{};
			EResourceState State{EResourceState::Unknown};
		};
		TArray<SubResState> m_SubResStates;
		EResourceState m_CurrentState{ EResourceState::Unknown };
		EResourceState m_TargetState{ EResourceState::Unknown }; // the state at next pass input
	};
#pragma endregion
}