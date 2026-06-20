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

	TaskGraph::TaskGraph() {
	}

	TaskGraph::~TaskGraph() {
	}

	void TaskGraph::AddNode(TUniquePtr<TaskNode>&& InNode) {
		const uint32 NodeIndex = TaskNodes.Size();
		TaskNodes.PushBack(MoveTemp(InNode));
		TaskNodes.Back()->IndexInGraph = (TaskNodeIndex)NodeIndex;
		NumPendingTasks.fetch_add(1, std::memory_order_release);
	}

	void TaskGraph::Connect(TaskNode* Left, TaskNode* Right) {
		CHECK(TaskNodes[Left->IndexInGraph].Get() == Left);
		CHECK(TaskNodes[Right->IndexInGraph].Get() == Right);
		CHECK(Left != Right);
		Left->Exits.Add(Right->IndexInGraph);
		++Right->NumEnters;
	}

	bool TaskGraph::CheckCircle() const {
		// Get degrees, and enqueue nodes without any entries.
		TaskNodeIndex NumNodes = (TaskNodeIndex)TaskNodes.Size();
		if(NumNodes == 0) {
			return false;
		}
		TArray<bool> NodeVisited(NumNodes); // Whether node visited.
		TArray<uint32> NodeEnters(NumNodes); // Num enter nodes of per node.
		TRingQueue<TaskNodeIndex> ZeroDegreeNodes(NumNodes); // Nodes without any entries.
		for (TaskNodeIndex i = 0; i < NumNodes; ++i) {
			const TaskNodeIndex InDegree = TaskNodes[i]->NumEnters;
			NodeEnters[i] = InDegree;
			if (0u == InDegree) {
				ZeroDegreeNodes.Enqueue(i);
			}
		}
		TaskNodeIndex NodeIndex;
		while(ZeroDegreeNodes.Dequeue(NodeIndex)) {
			if(NodeVisited[NodeIndex]) {
				return true;
			}
			NodeVisited[NodeIndex] = true;
			--NumNodes;
			for(const TaskNodeIndex Exit: TaskNodes[NodeIndex]->Exits) {
				if(--NodeEnters[Exit] == 0) {
					ZeroDegreeNodes.Enqueue(Exit);
				}
			}
		}
		return NumNodes != 0;
	}

	void TaskGraph::WaitUntilComplete() {
		ASSERT(!CheckCircle(), "Task graph circle checked!");
		const uint32 NumNodes = TaskNodes.Size();
		for(TaskNodeIndex i=0; i<NumNodes; ++i) {
			TaskNodes[i]->NumEntersRest.store(TaskNodes[i]->NumEnters, std::memory_order_relaxed);
		}
		for(TaskNodeIndex i=0; i<NumNodes; ++i) {
			if (0u == TaskNodes[i]->NumEnters) {
				EnqueueNode(i);
			}
		}
		// Run in current thread if possible.
		while(NumPendingTasks.load(std::memory_order_acquire) != 0) {
			Engine::XXThreadPool::Instance()->ExecutePendingTask(ETaskType::Worker);
		}
	}

	void TaskGraph::EnqueueNode(TaskNodeIndex Index) {
		EnqueueWorkerThreadTask([this, Index]() {
			TaskNode* Node = TaskNodes[Index].Get();
			Node->ExecuteTask();
			NumPendingTasks.fetch_sub(1, std::memory_order_release);
			for (const TaskNodeIndex ExitIndex : Node->Exits) {
				TaskNode* ExitNode = TaskNodes[ExitIndex].Get();
				const TaskNodeIndex NumPrevEnters = ExitNode->NumEntersRest.fetch_sub(1, std::memory_order_acq_rel);
				if (1 == NumPrevEnters) {
					EnqueueNode(ExitIndex);
				}
			}
		});
	}
}
