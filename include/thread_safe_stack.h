#pragma once

#include <mutex>
#include <stack>
#include <optional>
#include "common.h"

template<typename T>
class ThreadSafeStack {
private:
    std::mutex m;
    std::stack<T> data;
public:
    ThreadSafeStack() = default;
    
    ThreadSafeStack(ThreadSafeStack&& other) {
        std::lock_guard<std::mutex> lk(other.m);
        data = std::move(other.data);
    }

    DISTALLOW_COPY_AND_ASSIGN(ThreadSafeStack);
    
    ~ThreadSafeStack() {
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