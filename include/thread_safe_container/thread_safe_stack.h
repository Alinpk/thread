#ifndef THREAD_SAFE_STACK_H
#define THREAD_SAFE_STACK_H
#include <mutex>
#include <stack>
#include <optional>
#include "util/pattern.h"

template<typename T>
class ThreadSafeStack {
public:
    ThreadSafeStack() = default;
    
    ThreadSafeStack(ThreadSafeStack&& other);

    DISTALLOW_COPY_AND_ASSIGN(ThreadSafeStack);
    
    // 调用者保证析构时数据不被访问
    ~ThreadSafeStack() = default;

    void Push(T elem);

    std::optional<T> TryPop();
private:
    std::mutex m;
    std::stack<T> data;
};

template<typename T>
ThreadSafeStack<T>::ThreadSafeStack(ThreadSafeStack&& other) 
{
    std::lock_guard<std::mutex> lk(other.m);
    data = std::move(other.data);
}

template<typename T>
void ThreadSafeStack<T>::Push(T elem) {
    std::lock_guard<std::mutex> lk(m);
    data.push(elem);
}

template<typename T>
std::optional<T> ThreadSafeStack<T>::TryPop() 
{
    std::lock_guard<std::mutex> lk(m);
    if (data.empty()) { 
        return std::nullopt;
    }

    std::optional<T> res(std::move(data.top()));
    data.pop();
    return res;
}
#endif