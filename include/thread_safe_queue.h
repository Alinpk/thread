#pragma once

#include "common.h"
#include <mutex>
#include <queue>

template<typename T>
class ThreadSafeQueue {
private:
    std::mutex m;
    std::queue<T> data;
public:
    ThreadSafeQueue() = default;
    
    ThreadSafeQueue(ThreadSafeQueue&& other) {
        std::lock_guard<std::mutex> lk(other.m);
        data = std::move(other.data);
    }

    DISTALLOW_COPY_AND_ASSIGN(ThreadSafeQueue);
    
    ~ThreadSafeQueue() {
        std::lock_guard<m> lk;
        data.clear();
    }

    void Push(T elem) {
        std::lock_guard<std::mutex> lk(m);
        data.push(elem);
    }

    std::option<T> TryPop() {
        std::lock_guard<std::mutex> lk(m);
        if (data.empty()) { 
            return std::nullopt;
        }

        std::option<T> res(std::move(data.top()));
        data.pop();
        return res;
    }
};