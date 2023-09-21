#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H
#include "util/pattern.h"
#include <optional>
#include <mutex>
#include <queue>
#include <chrono>
#include <condition_variable>
#include <stdint.h>

template<class T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;
    
    ThreadSafeQueue(ThreadSafeQueue&& other);

    DISTALLOW_COPY_AND_ASSIGN(ThreadSafeQueue);
    
    // 使用者保证销毁前无人访问
    ~ThreadSafeQueue() = default;

    void Push(T elem);

    std::optional<T> Pop(int32_t ms = 0);

private:
    std::optional<T> PopImm();
    std::optional<T> PopFor(uint32_t ms);

private:
    std::mutex m_mut;
    std::queue<T> m_data;
    std::condition_variable m_cv;
};

template<typename T>
ThreadSafeQueue<T>::ThreadSafeQueue(ThreadSafeQueue&& other) {
    std::lock_guard<std::mutex> lk(other.m_mut);
    m_data = std::move(other.m_data);
}

template<typename T>
void ThreadSafeQueue<T>::Push(T elem) {
    std::lock_guard<std::mutex> lk(m_mut);
    m_data.push(elem);
    m_cv.notify_all();
}

template<typename T>
std::optional<T> ThreadSafeQueue<T>::Pop(int32_t ms) 
{
    if (ms < 0) {
        return PopImm();
    } else {
        return PopFor(ms);
    }
}

template<typename T>
std::optional<T> ThreadSafeQueue<T>::PopImm()
{
    std::lock_guard<std::mutex> lk(m_mut);
    if (m_data.empty()) { 
        return std::nullopt;
    }

    std::optional<T> res(std::move(m_data.front()));
    m_data.pop();
    return res;
}

template<typename T>
std::optional<T> ThreadSafeQueue<T>::PopFor(uint32_t ms)
{
    std::unique_lock<std::mutex> lk(m_mut);
    if (m_cv.wait_for(lk, std::chrono::milliseconds(ms), [this]() { return !m_data.empty(); })) {
        auto res(std::move(m_data.front()));
        m_data.pop();
        return res;
    } else {
        return std::nullopt;
    }
}
#endif