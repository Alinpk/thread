#include "timer_thread_test.h"
#include "../test_util.h"
#include <condition_variable>
#include <util/period.h>
#include <chrono>
#include <cstdint>
#include <iostream>

std::mutex mtx;
std::condition_variable cv;
std::atomic<uint8_t> g_wakeup = 0;
std::atomic<bool> g_stop = false;

// 另一种方式是用事件通知去告知所有线程
// 比如创建n个管道来管理n个周期性线程（第n+1个线程则是定时器，定期休眠）
void Regular()
{
    while (!g_stop.load())
    {
        static int cnt = 0;
        {
            std::unique_lock<std::mutex> lck(mtx);
            ++g_wakeup;
            cv.notify_one();
        }
        ++cnt;
        if (cnt == 100) {
            std::cout << "1s pass\n" << std::endl;
            cnt = 0;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::cout << "break regular" << std::endl;
}

void Timer()
{
    while (!g_stop.load())
    {
        static int cnt = 0;
        {
            std::cout << cnt <<std::endl;
            ++cnt;
            std::unique_lock<std::mutex> lck(mtx);
            cv.wait(lck, []()
                    { return g_wakeup > 0; });
            std::cout << "timer wake up " << cnt << "times\n";
            --g_wakeup;
        }
        MutiThreadTimerWheel::instance().DoTick();
        std::cout << "do tick finish\n\n";
    }
    std::cout << "break timer" << std::endl;
}

TEST(WorkShop, TimerWheelInThreadPool)
{
    ThreadPool threadPool;

    threadPool.SignUpJob([](){ Regular(); });
    threadPool.SignUpJob([](){ Timer(); });
    double ms = 0;
    PeriodCount<double, std::milli> pms;
    {
        auto cb1 = [&]() { pms.End(); ms = pms.Count(); };
        pms.Start();
        auto key1 = MutiThreadTimerWheel::instance().Insert({std::move(cb1), 1000});
    }

    std::atomic<uint8_t> numOpTest = 0;
    {
        auto cb1 = [&]() { numOpTest += 10; };
        auto cb2 = [&]() { numOpTest += 40; };
        auto key1 = MutiThreadTimerWheel::instance().Insert({std::move(cb1), 500});
        auto key2 = MutiThreadTimerWheel::instance().Insert({std::move(cb2), 300});
    }

    EXPECT_TRUE_FOR_X_MS(1000, numOpTest == 50);
    EXPECT_TRUE_FOR_X_MS(2000, ms > 1000);
    std::cout << "numOpTest:" << static_cast<uint32_t>(numOpTest.load()) << std::endl;
    std::cout << "ms:" << ms << std::endl;
    g_stop.store(true);
}