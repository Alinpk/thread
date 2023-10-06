#ifndef SIMPLE_THREAD_POOL_H
#define SIMPLE_THREAD_POOL_H

#include <atomic>
#include <functional>
#include <vector>
#include <thread>
#include "thread_safe_container/thread_safe_queue.h"

// 利用RAII协助线程管理
class JoinThreads
{
public:
    JoinThreads(std::vector<std::thread> &threads);
    ~JoinThreads();
private:
    std::vector<std::thread> &joiner;
};

class ThreadPool
{
public:
    ThreadPool();
    ThreadPool(uint8_t threadsNum);

    ~ThreadPool();

    DISTALLOW_COPY_AND_MOVE(ThreadPool);

    template <typename Functype>
    void SignUpJob(Functype func);

private:
    void WorkThread();

private:
    std::atomic<bool> done;
    ThreadSafeQueue<std::function<void()>> workQueue;
    std::vector<std::thread> threads;
    JoinThreads joiner;
};

template <typename Functype>
void ThreadPool::SignUpJob(Functype func)
{
    workQueue.Push([cb = std::move(func)]() { cb(); });
}
#endif