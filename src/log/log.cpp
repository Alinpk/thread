#include "log/log.h"

Log& GetInstance()
{
    static Log instance;
    return instance;
}

bool Init(string file, uint32_t logQueue, uint32_t logBufSize, int splitLine)
{}

void WriteLog(int level, string message, std::source_location location)
{}

void Flush()
{}