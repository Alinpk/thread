#pragma once

#include "common.h"
#include <optional>
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
    
    // 使用者保证销毁前无人访问
    ~ThreadSafeQueue() = default;

    void Push(T elem) {
        std::lock_guard<std::mutex> lk(m);
        data.push(elem);
    }

    std::optional<T> TryPop() {
        std::lock_guard<std::mutex> lk(m);
        if (data.empty()) { 
            return std::nullopt;
        }

        std::optional<T> res(std::move(data.front()));
        data.pop();
        return res;
    }
};