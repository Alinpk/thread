#include "workshop/simple_thread_pool.h"

JoinThreads::JoinThreads(std::vector<std::thread> &threads)
    : joiner{threads}
{
}

JoinThreads::~JoinThreads()
{
    for (auto &thread : joiner)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }
}

ThreadPool::ThreadPool()
    : done{false}, joiner(threads)
{
    const size_t SUPPORT_THREADS = std::thread::hardware_concurrency();
    const size_t numOfThreads = SUPPORT_THREADS ? SUPPORT_THREADS * 2 : 1;
    for (size_t i = 0; i < numOfThreads; ++i)
    {
        threads.emplace_back(std::thread(&ThreadPool::WorkThread, this));
    }
}

ThreadPool::ThreadPool(uint8_t threadsNum)
    : done{false}, joiner(threads)
{
    for (size_t i = 0; i < threadsNum; ++i)
    {
        threads.emplace_back(std::thread(&ThreadPool::WorkThread, this));
    }
}

ThreadPool::~ThreadPool()
{
    done = true;
}

void ThreadPool::WorkThread()
{
    while (!done)
    {
        if (auto work = workQueue.Pop(); work.has_value())
        {
            (work.value())();
        }
        else
        {
            std::this_thread::yield();
        }
    }
}