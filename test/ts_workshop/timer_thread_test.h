/*
An example of how to use timer in a threads-pool
*/

#include "util/timer_wheel.h"
#include "workshop/simple_thread_pool.h"
#include <unordered_map>
#include <unordered_set>

namespace
{
    // 时间槽间隔10ms（最小定时器时间间隔）
    constexpr uint32_t SLOT_INTERVAL_MS = 10;
    // 时间槽数量（大小会影响到散列的效果）
    constexpr uint16_t SLOT_NUM = 20;
}

using CbT = std::function<void(void)>;

/*
！
突然发现，如果使用一把锁进行Insert/Delete/Dotick操作，会导致Dotick有可能会执行时间较长
但Insert/Delete使用者预期应该快速结束

使用双队列可以解决这个问题，让Insert/Delete完成后快速返回，每次Dotick时，加锁其中一个队列，并将容器切换到另一个队列，
之后依次进行Insert、Delete、Dotick三种操作

但这样当前实现又个问题，Insert返回的Key是TimerWheel内部产生的，用以上方法虽然解决时间问题，又会产生一个Key无法及时生成的问题，
以及delete无法及时反馈的问题，目前先使用一张映射表缓解本问题
*/
using MKeyT = unsigned long long;
class MutiThreadTimerWheel {
public:
    static MutiThreadTimerWheel& instance()
    {
        static MutiThreadTimerWheel m_instance;
        return m_instance;
    }

    std::optional<MKeyT> Insert(Task<CbT>&& task);
    bool Delete(MKeyT taskId);
    void DoTick();
private:
    MutiThreadTimerWheel() { m_key = 0; }
    TimerWheel<CbT, SLOT_NUM, SLOT_INTERVAL_MS> m_timerWheel;
    MKeyT m_key; 
    std::unordered_map<MKeyT, Task<CbT>> m_toInsert;
    std::unordered_set<MKeyT> m_toDelete;
    std::unordered_map<MKeyT, KeyT> m_keyMap;
    std::mutex m_mt;
};

std::optional<MKeyT> MutiThreadTimerWheel::Insert(Task<CbT>&& task)
{
    std::lock_guard l(m_mt);
    // 这里m_key挂满会死循环，偷懒先不考虑这种情况，可以用一个计数解决
    while (m_keyMap.count(++m_key) != 0);
    // 占位 这里0后续真正插入时替换
    m_keyMap.insert({m_key, 0});
    auto wrapper = [cb = std::move(task.cb), this] () {
        cb();
        std::lock_guard l(m_mt);
        this->m_keyMap.erase(m_key);
        this->m_toDelete.erase(m_key);
    };
    task.cb = std::move(wrapper);
    m_toInsert.insert({m_key, std::move(task)});
    return m_key;
}

bool MutiThreadTimerWheel::Delete(MKeyT taskId)
{
    std::lock_guard l(m_mt);
    if (auto it = m_keyMap.find(taskId); it == m_keyMap.end())
    {
        return false;
    }
    else
    {
        m_toDelete.insert(taskId);
        return true;
    }
}

void MutiThreadTimerWheel::DoTick()
{
    {
    std::lock_guard l(m_mt);
    for (auto it = m_toInsert.begin(); it != m_toInsert.end(); ++it)
    {
        auto key = m_timerWheel.Insert(std::move(it->second));
        if (!key)
        {
            printf("%llu insert failed\n", it->first);
            m_keyMap.erase(it->first);
        }
        else
        {
            m_keyMap[it->first] = key.value();
        }
    }
    m_toInsert.clear();

    for (auto it = m_toDelete.begin(); it != m_toDelete.end(); ++it)
    {
        m_timerWheel.Delete(m_keyMap[*it]);
        m_keyMap.erase(*it);
    }
    m_toDelete.clear();
    }
    m_timerWheel.DoTick();
}