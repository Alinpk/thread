#include "thread_safe_container/thread_safe_queue.h"
#include "thread_safe_container/thread_safe_stack.h"
#include <vector>
#include <future>
#include <thread>
#include <algorithm>
#include <gtest/gtest.h>

TEST(BaseTest, Queue)
{
    ThreadSafeQueue<int> q;
    EXPECT_EQ(q.Pop(), std::nullopt);

    {
        const std::vector<int> testVal{1, 2, 3, 4};
        for (auto i : testVal) {
            q.Push(i);
        }

        std::vector<int> cmpVal;
        std::optional<int> tmp;
        while ((tmp = q.Pop())) {
            cmpVal.push_back(tmp.value());
        }
        EXPECT_EQ(cmpVal, testVal);
    }

    // block test
    {
        int val = 20;
        std::thread t([&q, val]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            q.Push(val);
        });
        // note: follow step will lead to fail for async may not real async
        // (void)std::async(std::launch::async, [&q, val]() {
        //     std::this_thread::sleep_for(std::chrono::milliseconds(30));
        //     q.Push(val);
        // });
        EXPECT_EQ(q.Pop(), std::nullopt);
        EXPECT_EQ(q.Pop(50), val);
        t.join();
    }
}

TEST(BaseTest, Stack)
{
    ThreadSafeStack<int> q;
    EXPECT_EQ(q.TryPop(), std::nullopt);

    {
        const std::vector<int> testVal{1, 2, 3, 4};
        for (auto i : testVal) {
            q.Push(i);
        }

        std::vector<int> cmpVal;
        std::optional<int> tmp;
        while ((tmp = q.TryPop())) {
            cmpVal.push_back(tmp.value());
        }
        std::reverse(cmpVal.begin(), cmpVal.end());
        EXPECT_EQ(cmpVal, testVal);
    }
}