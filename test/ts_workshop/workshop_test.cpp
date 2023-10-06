#include "workshop/simple_thread_pool.h"
#include "util/timer_wheel.h"
#include "../test_util.h"
#include <iostream>
#include <atomic>
TEST(WorkShop, ThreadPoolTest)
{
    ThreadPool threadPool;

    std::atomic<int> num = 0;
    auto plus = [&num]() {
        ++num;
    };

    int loopTime = 1000;
    while (loopTime > 0) {
        threadPool.SignUpJob(plus);
        --loopTime;
    }

    EXPECT_TRUE_FOR_X_MS(10000, num == 1000);
}