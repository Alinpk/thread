#pragma once
#include <stdint.h>
#include <array>
#include <unordered_map>
#include <optional>
#include <numeric>
#include "util/pattern.h"

template<typename CallBackFuncT>
struct Task {
    CallBackFuncT cb; // 回调函数
    uint32_t interval; // 定时器间隔
};

using KeyT = uint32_t;

namespace {
using SlotKeyT = uint16_t;
// 在Slot中的Interval并非是时间，而是指还需跳过iv次dotick调用才会触发任务
template<typename T>
class TimerWheelSlot {
public:
    TimerWheelSlot() = default;
    std::optional<SlotKeyT> Insert(Task<T>&& task);
    bool Delete(SlotKeyT taskId);
    void DoTick();

private:
    std::unordered_map<SlotKeyT, Task<T>> m_task;
    SlotKeyT m_keyGenerator{TimerWheelSlot<T>::INVALID_KEY};
    static const SlotKeyT INVALID_KEY = std::numeric_limits<SlotKeyT>::max();
};
} // namespace

// 用户自行保证Dotick期间不做任务的删除/插入
// 或是修改触发方式，通过队列形式保证先检查任务，Dotick期间做实际插入
// 不要插入立刻触发的任务
template<typename T, uint16_t SLOT_NUM, uint32_t SLOT_INTERVAL>
class TimerWheel {
public:
    TimerWheel() = default;
    DISTALLOW_COPY_AND_MOVE(TimerWheel);

    std::optional<KeyT> Insert(Task<T>&& task);
    bool Delete(KeyT taskId);
    void DoTick();

private:
    // 预处理任务，检查时间是否合法，合法则对任务时间做修改
    std::optional<size_t> ProcTask(Task<T>& task);
    KeyT GenerateKey(uint16_t slotNum, SlotKeyT slotKey);
    std::pair<uint16_t, SlotKeyT> DecodeKey(KeyT key);
private:
    std::array<TimerWheelSlot<T>, SLOT_NUM> m_timerWheel;
    size_t m_curSlot{0U}; // 当前指向的槽位
};

template<typename T>
std::optional<SlotKeyT> TimerWheelSlot<T>::Insert(Task<T>&& task)
{
    //  max size
    if (m_task.size() == INVALID_KEY) {
        return std::nullopt;
    }

    typename std::unordered_map<SlotKeyT, Task<T>>::iterator it;
    do {
        ++m_keyGenerator;
        it = m_task.find(m_keyGenerator);
    } while (it != m_task.end());

    m_task.emplace(std::make_pair(m_keyGenerator, std::move(task)));
    return m_keyGenerator;
}

template<typename T>
bool TimerWheelSlot<T>::Delete(SlotKeyT taskId)
{
    if (auto it = m_task.find(taskId); it != m_task.end()) {
        m_task.erase(it);
        return true;
    } else {
        return false;
    }
}

template<typename T>
void TimerWheelSlot<T>::DoTick()
{
    for(auto it = m_task.begin(); it != m_task.end();) {
        if (it->second.interval == 0U) {
            printf("%s%d\n", __FILE__, __LINE__);
            it->second.cb();
            // 关联式容器 遍历过程中删除需要用到erase返回值，否则会发生未定义行为
            it = m_task.erase(it);
        } else {
            printf("%s%d\n", __FILE__, __LINE__);
            it->second.interval -= 1;
            ++it;
        }
        printf("%s%d\n", __FILE__, __LINE__);
    }
}

template<typename T, uint16_t SLOT_NUM, uint32_t SLOT_INTERVAL>
std::optional<size_t> TimerWheel<T, SLOT_NUM,SLOT_INTERVAL>::ProcTask(Task<T>& task) {
    if (task.interval % SLOT_INTERVAL != 0U) {
        return std::nullopt;
    }
    task.interval /= SLOT_INTERVAL;
    size_t slotNum = (task.interval % SLOT_NUM + m_curSlot) % SLOT_NUM;
    task.interval /= SLOT_NUM;
    return slotNum;
}

template<typename T, uint16_t SLOT_NUM, uint32_t SLOT_INTERVAL>
KeyT TimerWheel<T, SLOT_NUM,SLOT_INTERVAL>::GenerateKey(uint16_t slotNum, SlotKeyT slotKey)
{
    return (slotNum << 16) + slotKey;
}

template<typename T, uint16_t SLOT_NUM, uint32_t SLOT_INTERVAL>
std::pair<uint16_t, SlotKeyT> TimerWheel<T, SLOT_NUM,SLOT_INTERVAL>::DecodeKey(KeyT key)
{
    return {key >> 16, key & 0x0000FFFF};
}

template<typename T, uint16_t SLOT_NUM, uint32_t SLOT_INTERVAL>
std::optional<KeyT> TimerWheel<T, SLOT_NUM,SLOT_INTERVAL>::Insert(Task<T>&& task) {
    auto slotNum = ProcTask(task);
    if (!slotNum.has_value()) {
        // 时间间隔不合法
        return std::nullopt;
    }
    auto slotKey = m_timerWheel[slotNum.value()].Insert(std::move(task));
    if (!slotKey.has_value()) {
        return std::nullopt;
    }

    return GenerateKey(slotNum.value(), slotKey.value());
}

template<typename T, uint16_t SLOT_NUM, uint32_t SLOT_INTERVAL>
bool TimerWheel<T, SLOT_NUM,SLOT_INTERVAL>::Delete(KeyT taskId)
{
    auto keyPair = DecodeKey(taskId);
    return m_timerWheel[keyPair.first].Delete(keyPair.second);
}

template<typename T, uint16_t SLOT_NUM, uint32_t SLOT_INTERVAL>
void TimerWheel<T, SLOT_NUM,SLOT_INTERVAL>::DoTick()
{
    printf("%s%d\n", __FILE__, __LINE__);
    m_timerWheel[m_curSlot].DoTick();
    printf("%s%d\n", __FILE__, __LINE__);
    ++m_curSlot;
}