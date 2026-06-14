#pragma once
#include "Core/Public/Defines.h"
#include "Core/Public/TArray.h"
#include "Core/Public/Func.h"
#include "Core/Public/TUniquePtr.h"
#include <thread>
#include <atomic>

namespace Engine{

    uint32 CurrentThreadIndex();

    // Runnable task for thread pool
    class ThreadTask {
    public:
        virtual ~ThreadTask() = default;
        virtual void Execute(uint32 ThreadIndex) {/* Do nothing */}
        virtual void Interrupt() { /* Do nothing */}
    };

    typedef TUniquePtr<ThreadTask> ThreadTaskPtr;

    template<class Func> class ThreadTaskFunc: public ThreadTask {
    public:
        ThreadTaskFunc(Func&& InFunc) :FuncBody(MoveTemp(InFunc)) {}
        virtual void Execute(uint32 ThreadIndex) override { FuncBody(); }
    private:
        Func FuncBody;
    };

    // Sync
    class ThreadFence {
    public:
        ThreadFence();
        ~ThreadFence();
        void Reset();
        void Signal();
        void Wait();
        bool IsSignaled() const;
    private:
        std::atomic_bool StatusFlag;
    };

    enum class ETaskType : uint8 {
        GameThread,
        Worker,
        MaxNum,
    };

    class XXThreadPool {
        SINGLETON_INSTANCE(XXThreadPool);
        XXThreadPool();
        ~XXThreadPool();
    public:
        void Tick();
        void EnqueueTaskPtr(ETaskType InType, ThreadTaskPtr&& InTask);
        uint32 GetNumThreads()const;
        bool ExecutePendingTask(ETaskType InType);
    private:
        TArray<std::thread> AllThreads; // All threads except main thread
        std::atomic_bool bRunning;
        void WorkerThreadLoop();
    };

    template<class Func> void EnqueueWorkerThreadTask(Func&& InFunc) {
	    XXThreadPool::Instance()->EnqueueTaskPtr(ETaskType::Worker, ThreadTaskPtr(new ThreadTaskFunc<Func>(ForwardTemp(InFunc))));
    }

    template<class Func> void EnqueueGameThreadTask(Func&& InFunc) {
        XXThreadPool::Instance()->EnqueueTaskPtr(ETaskType::GameThread, ThreadTaskPtr(new ThreadTaskFunc<Func>(ForwardTemp(InFunc))));
    }
}
