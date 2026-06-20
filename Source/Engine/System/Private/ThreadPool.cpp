#include "System/Public/ThreadPool.h"
#include "System/Public/ConfigManager.h"
#include "Core/Public/Log.h"
#include "Core/Public/EnumClass.h"
#include <concurrentqueue.h>
#include <lightweightsemaphore.h>
#ifdef _WIN32
#include <Windows.h>
#include <processthreadsapi.h>
#else
#include <pthread.h>
#endif

namespace Engine{

	inline void SetCurrentThreadName(const XStringView Name) {
#ifdef _WIN32
		SetThreadDescription(GetCurrentThread(), std::wstring(Name.begin(), Name.end()).c_str());
#elif __linux__
		pthread_setname_np(pthread_self(), Name.c_str());
#elif __APPLE__
		pthread_setname_np(Name.c_str());
#endif
	}

	static thread_local uint32 GThreadIndex;
	TStaticArray<moodycamel::ConcurrentQueue<ThreadTaskPtr>, EnumCast(ETaskType::MaxNum)> GPendingTasks;
	moodycamel::LightweightSemaphore GWorkerSmp;
	TArray<std::thread> AllThreads; // All threads except main thread
	std::atomic_bool bRunning;

	void SetCurrentThreadIndex(uint32 Index) {
		GThreadIndex = Index;
	}

	uint32 CurrentThreadIndex() {
		return GThreadIndex;
	}


	ThreadFence::ThreadFence() {
		Reset();
	}

	ThreadFence::~ThreadFence() {
	}

	void ThreadFence::Reset() {
		StatusFlag.store(false, std::memory_order_release);
	}

	void ThreadFence::Signal() {
		StatusFlag.store(true, std::memory_order_release);
	}

	void ThreadFence::Wait() {
		while(!IsSignaled()) {
			// Wait();
		}
	}

	bool ThreadFence::IsSignaled() const {
		return StatusFlag.load(std::memory_order_acquire);
	}

	XXThreadPool::XXThreadPool() {
		// Determine num of extra threads.
		const uint32 NumThreadsHardware = std::thread::hardware_concurrency();
		if (NumThreadsHardware == 0) {
			LOG_WARNING("std::thread::hardware_concurrency returns 0!");
		}
		const uint32 NumThreadsConfig = ConfigMgr::Instance().GetEngineConfig().NumSubThreads;
		const uint32 NumThreads = NUM_MIN(NumThreadsConfig, NumThreadsHardware - 1);

		// Launch threads
		bRunning.store(true, std::memory_order_release);
		// Setup game thread
		SetCurrentThreadName("GameThread");
		SetCurrentThreadIndex(0);
		// Launch worker thread
		if(NumThreads > 0) {
			AllThreads.Reserve(NumThreads);
			for (uint32 i = 0; i < NumThreads; ++i) {
				const uint32 WorkerIndex = i;
				AllThreads.Add(std::thread{ [this, WorkerIndex]() {
					const XString ThreadName = StringFormat("Worker%u", WorkerIndex);
					SetCurrentThreadName(ThreadName);
					SetCurrentThreadIndex(WorkerIndex + 1);
					WorkerThreadLoop();
				} });
			}
		}
	}

	XXThreadPool::~XXThreadPool() {
		bRunning.store(false, std::memory_order_release);
		//GWorkerFence.Signal();
		GWorkerSmp.signal(AllThreads.Size());
		for(uint32 i=0; i<AllThreads.Size(); ++i) {
			AllThreads[i].join();
		}
	}

	void XXThreadPool::Tick() {
		// Handle game thread tasks
		{
			constexpr uint32 GameThreadIndex = 0;
			ThreadTaskPtr Task;
			while (GPendingTasks[EnumCast(ETaskType::GameThread)].try_dequeue(Task)) {
				Task->Execute(GameThreadIndex);
			}
		}
	}

	void XXThreadPool::EnqueueTaskPtr(ETaskType InType, ThreadTaskPtr&& InTask) {
		GPendingTasks[EnumCast(InType)].enqueue(ForwardTemp(InTask));
		if(InType == ETaskType::Worker) {
			GWorkerSmp.signal(1);
		}
	}

	uint32 XXThreadPool::GetNumThreads() const {
		return AllThreads.Size() + 1;
	}

	bool XXThreadPool::ExecutePendingTask(ETaskType InType) {
		ThreadTaskPtr Task;
		if (GPendingTasks[EnumCast(InType)].try_dequeue(Task)) {
			Task->Execute(CurrentThreadIndex());
			return true;
		}
		return false;
	}

	void XXThreadPool::WorkerThreadLoop() {
		while (bRunning.load(std::memory_order_relaxed)) {
			GWorkerSmp.wait();
			ExecutePendingTask(ETaskType::Worker);
		}
	}
}
