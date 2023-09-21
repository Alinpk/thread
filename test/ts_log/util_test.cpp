#include "log/log.h"
#include <gtest/gtest.h>
#include <iostream>

TEST(Log, FmtTest)
{
    auto s = LOG_DEBUG("im {}", "alan");
    std::cout << s;
}