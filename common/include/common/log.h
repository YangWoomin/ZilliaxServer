
#ifndef __ZS_COMMON_LOG_H__
#define __ZS_COMMON_LOG_H__

#include    <csignal>
#include    <cstdarg>
#include    <functional>

namespace zs
{
namespace common
{
    enum LogLevel
    {
        LOGLEVEL_DEBUG     = 1,
        LOGLEVEL_INFO      = 2,
        LOGLEVEL_WARN      = 3,
        LOGLEVEL_ERROR     = 4,
        LOGLEVEL_FATAL     = 5,
    };

    class Logger
    {
    public:
        static void Write(const char* category, const char* file, int line, LogLevel level, const char* fmt, ...)
        {
            Messenger& messenger = GetMessenger();
            if (nullptr != messenger)
            {
                va_list args;
                va_start(args, fmt);
                
                messenger(category, file, line, level, fmt, args);

                va_end(args);
            }
        }

        using Messenger = std::function<void(const char* /*category*/, const char* /*file*/, int /*line*/, LogLevel, const char* /*fmt*/, va_list /*args*/)>;
        static Messenger& GetMessenger()
        {
            static Messenger messenger;
            return messenger;
        }
    };
}
}

#define ZS_LOG_FATAL(category, fmt, ...) do {\
    zs::common::Logger::Write(#category, __FILE__, __LINE__, zs::common::LogLevel::LOGLEVEL_FATAL, fmt, ##__VA_ARGS__); \
    raise(SIGABRT); \
} while(0)
#define ZS_LOG_ERROR(category, fmt, ...)    zs::common::Logger::Write(#category, __FILE__, __LINE__, zs::common::LogLevel::LOGLEVEL_ERROR, fmt, ##__VA_ARGS__)
#define ZS_LOG_WARN(category, fmt, ...)     zs::common::Logger::Write(#category, __FILE__, __LINE__, zs::common::LogLevel::LOGLEVEL_WARN, fmt, ##__VA_ARGS__)
#define ZS_LOG_INFO(category, fmt, ...)     zs::common::Logger::Write(#category, __FILE__, __LINE__, zs::common::LogLevel::LOGLEVEL_INFO, fmt, ##__VA_ARGS__)
#ifdef _DEBUG
#define ZS_LOG_DEBUG(category, fmt, ...)    zs::common::Logger::Write(#category, __FILE__, __LINE__, zs::common::LogLevel::LOGLEVEL_DEBUG, fmt, ##__VA_ARGS__)
#else // _DEBUG
#define ZS_LOG_DEBUG(category, fmt, ...)
#endif // _DEBUG

#endif // __ZS_COMMON_LOG_H__
