#pragma once

#include <mutex>
#include <thread>

typedef std::mutex Mutex;
typedef std::lock_guard<Mutex> MutexLock;
typedef std::thread Thread;

class TaskGraph;
class TaskNode;