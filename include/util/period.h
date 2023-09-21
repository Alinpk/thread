#ifndef PERIOD_H
#define PERIOD_H

#include <chrono>

// default = second
// ratio<n, m> -> n/m second
template<class T, class Scale = std::ratio<1>>
class PeriodCount {
public:
    PeriodCount() = default;

    PeriodCount(T* counter)
    {
        m_flag = true;
        m_counter = counter;
        Start();
    }

    ~PeriodCount()
    {
        if (m_flag) {
            End();
            std::chrono::duration<T, Scale> period = m_end - m_start;
            *m_counter += period.count();
        }
    }

    void Start()
    {
        m_start = std::chrono::steady_clock::now();
    }

    void Cancel()
    {
        m_flag = false;
    }

    void End()
    {
        m_end = std::chrono::steady_clock::now();
    }

    auto Count() -> T
    {
        std::chrono::duration<T, Scale> period = m_end - m_start;
        return period.count();
    }

private:
    std::chrono::time_point<std::chrono::steady_clock> m_start;
    std::chrono::time_point<std::chrono::steady_clock> m_end;
    T* m_counter{nullptr}; // 
    bool m_flag{false}; // 用来标识counter指针是否有效
};
#endif