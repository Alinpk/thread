#include "log/log.h"
#include <gtest/gtest.h>
#include <unistd.h>
#include <iostream>

void deleteDirectory(const std::filesystem::path& dir);

TEST(Log, FmtTest)
{
    // get current work directory
    auto path = getcwd(NULL,0);
    std::filesystem::path fullPath(path);
    free(path);

    fullPath.append("log_test_dir/test_log_");

    Log::GetInstance().Init(fullPath.c_str(), 3, 0);
    LOG_DEBUG("this log will not be recorded");
    LOG_INFO("Start record from here");
    LOG_WARN("Today is {}", "Fri");
    LOG_ERROR("Flush {} times", 1);
    // 后续补匹配检查
    deleteDirectory(fullPath.parent_path());
}

// [D] | 2023-09-22:16:31 |this log will not be recorded
// [I] | 2023-09-22:16:31 |Start record from here
// [W] | 2023-09-22:16:31 |Today is Fri
// [E] | 2023-09-22:16:31 |Flush 1 times

void deleteDirectory(const std::filesystem::path& dir)
{
    for (const auto& entry : std::filesystem::directory_iterator(dir)) 
        std::filesystem::remove_all(entry.path());
    std::filesystem::remove(dir);
}