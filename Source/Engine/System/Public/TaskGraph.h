#pragma once
#include "Core/Public/Concurrency.h"
#include "Core/Public/Defines.h"
#include "Core/Public/TArray.h"
#include "Core/Public/Func.h"
#include "Core/Public/TUniquePtr.h"
#include "Core/Public/String.h"
#include <atomic>

namespace Engine {

	typedef uint16 TaskNodeIndex;
	class TaskNode{
	public:
		TaskNode();
		void SetName(XStringView InName);
		virtual ~TaskNode();
		virtual void ExecuteTask() = 0;
	private:
		friend class TaskGraph;
		static constexpr TaskNodeIndex INVALID_NODE = UINT16_MAX;
		XString Name;
		TArray<TaskNodeIndex> LinkedIndices; // Enters: [0, 1, ..., NumEnters-1, ], Exits: [NumEnters, NumEnters+1, ...]
		TaskNodeIndex IndexInGraph;
		TaskNodeIndex NumEnters;
		std::atomic<TaskNodeIndex> NumEntersRest;
		void AddEnter(TaskNodeIndex InIndex);
		void AddExit(TaskNodeIndex InIndex);
		TConstArrayView<uint16> GetExits() const;
	};

	using TaskFunc = XFunc<void>;
	class TaskNodeFunc: public TaskNode{
	public:
		TaskNodeFunc(TaskFunc&& InFunc);
		virtual void ExecuteTask() override;
	private:
		TaskFunc Func;
	};

	class TaskGraph {
	public:
		NON_COPYABLE(TaskGraph);
		NON_MOVEABLE(TaskGraph);
		TaskGraph();
		~TaskGraph();

		template<class T, class ...Args>
		T* CreateNode(XStringView TaskName, Args&&... InArgs);

		void AddNode(TUniquePtr<TaskNode>&& InNode);
		void Link(TaskNode* Left, TaskNode* Right);
		void WaitUntilComplete();
		void WaitUntilComplete2();
	private:
		TArray<TUniquePtr<TaskNode>> TaskNodes;
		std::atomic<TaskNodeIndex> NumPendingTasks;
		void EnqueueNode(TaskNodeIndex Index);
	};

	template <class T, class ... Args>
	T* TaskGraph::CreateNode(XStringView TaskName, Args&&... InArgs) {
		TUniquePtr<TaskNode> NodePtr{ new T(ForwardTemp(InArgs)...)};
		TaskNode* Node = NodePtr.Get();
		Node->SetName(TaskName);
		AddNode(MoveTemp(NodePtr));
		return (T*)Node;
	}
}
