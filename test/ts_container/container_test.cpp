#include "thread_safe_container/thread_safe_queue.h"
#include "thread_safe_container/thread_safe_stack.h"
#include <vector>
#include <algorithm>
#include <gtest/gtest.h>

TEST(BaseTest, Queue)
{
    ThreadSafeQueue<int> q;
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
        EXPECT_EQ(cmpVal, testVal);
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