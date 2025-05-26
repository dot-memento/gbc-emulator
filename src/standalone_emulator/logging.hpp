#pragma once

#include <sstream>
#include <chrono>
#include <deque>
#include <vector>


enum TLogLevel { logFATAL, logERROR, logWARN, logINFO, logDEBUG, logTRACE, logLevel_count };

#define LOG(level) if (level <= Log::reporting_level) Log().Get(level)

#define LOG_FATAL if constexpr (logFATAL <= Log::reporting_level) Log().Get(logFATAL)
#define LOG_ERROR if constexpr (logERROR <= Log::reporting_level) Log().Get(logERROR)
#define LOG_WARN  if constexpr (logWARN  <= Log::reporting_level) Log().Get(logWARN)
#define LOG_INFO  if constexpr (logINFO  <= Log::reporting_level) Log().Get(logINFO)
#define LOG_DEBUG if constexpr (logDEBUG <= Log::reporting_level) Log().Get(logDEBUG)
#define LOG_TRACE if constexpr (logTRACE <= Log::reporting_level) Log().Get(logTRACE)


inline const char* logLevelToString(TLogLevel level)
{
    static constexpr const char* log_level_to_string[] =
        {"FATAL", "ERROR", "WARN ", "INFO ", "DEBUG", "TRACE"};
    return log_level_to_string[level];
}

struct LogEntry
{
    std::time_t timestamp;
    TLogLevel level;
    std::string message;
};

class Log
{
public:
    static constexpr size_t max_message_log = 100;
    static constexpr TLogLevel reporting_level = logTRACE;

    using LogCallback = void(*)(const LogEntry&);
    inline static std::vector<LogCallback> log_callbacks;

    ~Log()
    {
        LogEntry entry
        {
            std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()),
            level_,
            os_.str()
        };

        for (auto callback : log_callbacks)
            callback(entry);

        auto& message_queue = getMessageLog_internal();
        if (message_queue.size() >= max_message_log)
            message_queue.pop_front();
        message_queue.push_back(std::move(entry));
    }

    Log() = default;
    Log(const Log&) = delete;
    Log& operator =(const Log&) = delete;

    constexpr std::ostringstream& Get(TLogLevel level = logINFO)
    {
        level_ = level;
        return os_;
    }

    static inline const std::deque<LogEntry>& getMessageLog()
    {
        return getMessageLog_internal();
    }

private:
    static inline std::deque<LogEntry>& getMessageLog_internal()
    {
        static std::deque<LogEntry> message_queue;
        return message_queue;
    }

    TLogLevel level_;
    std::ostringstream os_;
};
