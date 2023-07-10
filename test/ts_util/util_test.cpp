#include "util/decl_operator.h"
#include "util/timer_wheel.h"
#include <functional>
#include <gtest/gtest.h>


namespace {
int test_1() { return 1; }
int test_2() { return 2; }
#define TEST_FUNC(idx) test_##idx
#define CNT(idx) TEST_FUNC(idx)

} // namespace

TEST(Util, MacroTest)
{
    EXPECT_EQ(CNT(CNT_PARAM(one))(), 1);
    EXPECT_EQ(CNT(CNT_PARAM(one, two))(), 2);
}

TEST(Util, TimerWheelTest)
{
    using CallBackFuncT = std::function<void(void)>;
    constexpr uint32_t slotInterval = 10;
    constexpr uint16_t slotNum = 2;
    TimerWheel<CallBackFuncT, slotNum, slotInterval> tw;
    
    // invalid interval test
    {
        EXPECT_EQ(tw.Insert({[](){}, 5U}), std::nullopt);
    }

    int cnt = 0;
    // insert test
    {
        EXPECT_NE(tw.Insert({[&cnt](){ ++cnt; }, 0U}), std::nullopt);
        EXPECT_NE(tw.Insert({[&cnt](){ ++cnt; }, 10U}), std::nullopt);
        EXPECT_NE(tw.Insert({[&cnt](){ ++cnt; }, 10U}), std::nullopt);
        EXPECT_NE(tw.Insert({[&cnt](){ ++cnt; }, 20U}), std::nullopt);
    }

    // dotick test
    {
        tw.DoTick();
        EXPECT_EQ(cnt, 1);
        tw.DoTick();
        EXPECT_EQ(cnt, 3);
        tw.DoTick();
        EXPECT_EQ(cnt, 4);
    }
}