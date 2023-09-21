#ifndef SIMPLE_THREAD_POOL_H
#define SIMPLE_THREAD_POOL_H

#include <atomic>
#include <functional>
#include <vector>
#include <thread>
#include "thread_safe_container/thread_safe_queue.h"

class JoinThreads {
    std::vector<std::thread>& joiner;
public:
    JoinThreads(std::vector<std::thread>& threads)
    : joiner{threads}
    {}

    ~JoinThreads() {
        for (auto& thread : joiner) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }
};

class SimpleThreadPool {
private:
    std::atomic<bool> done;
    ThreadSafeQueue<std::function<void()>> workQueue;
    std::vector<std::thread> threads;
    JoinThreads joiner;

    void WorkThread() {
        // while (!done) {
        //     if (auto work = workQueue.TryPop(); work.has_value()) {
        //         (work.value())();
        //     } else {
        //         std::this_thread::yield();
        //     }
        // }
    }

public:
    SimpleThreadPool()
    : done{false}
    {
        const size_t SUPPORT_THREADS = std::thread::hardware_concurrency();
        const size_t NUMS_OF_THREADS = SUPPORT_THREADS ? 1 : SUPPORT_THREADS;
        for (size_t i = 0; i < NUMS_OF_THREADS; ++i) {
            // workQueue.Push(std::thread(&WorkThread));
        }
    }

    ~SimpleThreadPool() {
        done = true;
    }

    DISTALLOW_COPY_AND_MOVE(SimpleThreadPool);

    template<typename Functype>
    void SignUpJob(Functype func) {
        workQueue.Push([cb = std::move(func)]() { cb(); })
    }
};
#endif