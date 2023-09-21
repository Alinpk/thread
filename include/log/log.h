#ifndef LOG_H
#define LOG_H
#include <stdint.h>
#include <string>
#include <format>
#include <source_location>
#include "thread_safe_container/thread_safe_queue.h"

// #define LOG_DEBUG(fmt, ...) { Log::GetInstance().WriteLog(0, std::source_location::current(),  fmt, ##__VA_ARGS__); }
// #define LOG_INFO(fmt, ...)  { Log::GetInstance().WriteLog(1, std::source_location::current(),  fmt, ##__VA_ARGS__); }
// #define LOG_WARN(fmt, ...)  { Log::GetInstance().WriteLog(2, std::source_location::current(),  fmt, ##__VA_ARGS__); }
// #define LOG_ERROR(fmt, ...) { Log::GetInstance().WriteLog(3, std::source_location::current(),  fmt, ##__VA_ARGS__); }
#define LOG_DEBUG(fmt, ...) { WriteLog(0, std::source_location::current(),  fmt, ##__VA_ARGS__); }

constexpr char[] LOG_LEVEL{ 'D', 'I', 'W', 'E' };

// class Log {
// public:
//     static Log& GetInstance();
//     bool Init(string file, uint32_t logQueue, uint32_t logBufSize, int splitLine);
    
//     template <typename... Args>
//     void WriteLog(int level, std::source_location source, string fmt, Args&&... args);
    
//     void Flush();

// private:
//     Log();
//     virtual ~Log();

// private:
//     string m_filename;
//     string m_path;
//     int m_levelMask;
//     int m_lineLimits;
//     int m_singleLineCharLimits;
//     ThreadSafeQueue<string> m_logBuf;
// };

template <typename... Args>
string WriteLog(int level, std::source_location source, string fmt, Args&&... args)
{
    // if ( level < m_levelMask ) {
    //     return;
    // }
    // [level] | time | source | message
    fmt = "[{}] {} | {} |" + fmt + "\n";
    return std::format(fmt,
                    LOG_LEVEL[level], 
                    to_string(as_local(std::chrono::system_clock::now())), 
                    to_string(source),
                    args... );
}
#endif