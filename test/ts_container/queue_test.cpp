#include "thread_safe_container/thread_safe_queue.h"
#include <gtest/gtest.h>

TEST(BaseTest, IOtest)
{
    ThreadSafeQueue<int> q;
    EXPECT_EQ(q.TryPop(), std::nullopt);

    {
        constexpr int testVal = 10;
        q.Push(testVal);
        std::optional<int> val = q.TryPop();
        EXPECT_TRUE(val.has_value());
        EXPECT_EQ(val.value(), testVal);
    }
}