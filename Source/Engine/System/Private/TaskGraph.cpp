#include "System/Public/TaskGraph.h"
#include "System/Public/ThreadPool.h"
#include "Core/Public/TQueue.h"
#include <concurrentqueue.h>

namespace Engine {
	TaskNode::TaskNode() : IndexInGraph(UINT32_MAX), NumEnters(0){
	}

	void TaskNode::SetName(XStringView InName) {
		Name = InName;
	}

	TaskNode::~TaskNode(){
	}

	void TaskNode::AddEnter(TaskNodeIndex InIndex) {
		LinkedIndices.Add(InIndex);
		if(NumEnters + 1 < LinkedIndices.Size()) {
			LinkedIndices.SwapEle(NumEnters, LinkedIndices.Size() - 1);
		}
		++NumEnters;
		NumEntersRest.fetch_add(1, std::memory_order_release);
	}

	void TaskNode::AddExit(TaskNodeIndex InIndex) {
		LinkedIndices.Add(InIndex);
	}

	TConstArrayView<uint16> TaskNode::GetExits() const {
		if(NumEnters < LinkedIndices.Size()) {
			return TConstArrayView<uint16>(&LinkedIndices[NumEnters], LinkedIndices.Size() - NumEnters);
		}
		return {};
	}

	TaskNodeFunc::TaskNodeFunc(TaskFunc&& InFunc) : TaskNode(), Func(MoveTemp(InFunc)){
	}

	void TaskNodeFunc::ExecuteTask() {
		Func();
	}

	TaskGraph::TaskGraph() {
	}

	TaskGraph::~TaskGraph() {
	}

	void TaskGraph::AddNode(TUniquePtr<TaskNode>&& InNode) {
		const uint32 NodeIndex = TaskNodes.Size();
		TaskNodes.PushBack(MoveTemp(InNode));
		TaskNodes.Back()->IndexInGraph = NodeIndex;
		NumPendingTasks.fetch_add(1, std::memory_order_release);
	}

	void TaskGraph::Link(TaskNode* Left, TaskNode* Right) {
		Left->AddExit(Right->IndexInGraph);
		Right->AddEnter(Left->IndexInGraph);
	}

	void TaskGraph::WaitUntilComplete() {
		// Get degrees, and enqueue nodes no entry
		const uint32 NumNodes = TaskNodes.Size();
		TArray<std::atomic<TaskNodeIndex>> NodeEnterDegrees(NumNodes);
		moodycamel::ConcurrentQueue<TaskNodeIndex> ZeroDegreeNodes(NumNodes); // Nodes with no enters.
		for(TaskNodeIndex i=0; i< NumNodes; ++i) {
			const TaskNodeIndex InDegree = TaskNodes[i]->NumEnters;
			NodeEnterDegrees[i].store(InDegree, std::memory_order_relaxed);
			if(0u == InDegree) {
				ZeroDegreeNodes.enqueue(i);
			}
		}

		TaskNodeIndex NodeIndex;
		while(!ZeroDegreeNodes.try_dequeue(NodeIndex)) {
			// Execute task
			TaskNode* Node = TaskNodes[NodeIndex].Get();
			Engine::EnqueueWorkerThreadTask([this, Node, &ZeroDegreeNodes, &NodeEnterDegrees]() {
				Node->ExecuteTask();
				// Decrease enters of next node.
				TConstArrayView<TaskNodeIndex> Exits = Node->GetExits();
				for (const TaskNodeIndex NextIndex : Exits) {
					const TaskNodeIndex PrevNum = NodeEnterDegrees[NextIndex].fetch_sub(1, std::memory_order_acq_rel);
					if (PrevNum == 1) {
						ZeroDegreeNodes.try_enqueue(NextIndex);
					}
				}
			});
		}

		// Ring checking
		for(uint32 i=0; i<NumNodes; ++i) {
			if(NodeEnterDegrees[i] != 0) {
				LOG_WARNING("Circle detected in task graph, %u, %s", i, TaskNodes[i]->Name.c_str());
			}
		}
	}

	void TaskGraph::WaitUntilComplete2() {
		const uint32 NumNodes = TaskNodes.Size();
		for(TaskNodeIndex i=0; i<NumNodes; ++i) {
			if (0u == TaskNodes[i]->NumEnters) {
				EnqueueNode(i);
			}
		}
		while(NumPendingTasks.load(std::memory_order_acquire) != 0) {
			Engine::XXThreadPool::Instance()->ExecutePendingTask(ETaskType::Worker);
		}
	}

	void TaskGraph::EnqueueNode(TaskNodeIndex Index) {
		EnqueueWorkerThreadTask([this, Index]() {
			TaskNode* Node = TaskNodes[Index].Get();
			Node->ExecuteTask();
			NumPendingTasks.fetch_sub(1, std::memory_order_release);
			TConstArrayView<TaskNodeIndex> Exits = Node->GetExits();
			for (const TaskNodeIndex ExitIndex : Exits) {
				TaskNode* ExitNode = TaskNodes[ExitIndex].Get();
				const TaskNodeIndex NumPrevEnters = ExitNode->NumEntersRest.fetch_sub(1, std::memory_order_acq_rel);
				if (1 == NumPrevEnters) {
					EnqueueNode(ExitIndex);
				}
			}
		});
	}
}
