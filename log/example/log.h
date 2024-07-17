
#ifndef __ZS_COMMON_LOG_H__
#define __ZS_COMMON_LOG_H__

#include    <csignal>
#include    <cstdarg>
#include    <functional>

namespace zs
{
    enum LogLevel
    {
        LOG_LEVEL_DEBUG     = 1,
        LOG_LEVEL_INFO      = 2,
        LOG_LEVEL_WARN      = 3,
        LOG_LEVEL_ERROR     = 4,
        LOG_LEVEL_FATAL     = 5,
    };

    class Log
    {
    public:
        static void Write(const char* category, const char* file, int line, LogLevel level, const char* fmt, ...)
        {
            Writer& writer = GetWriter();
            if (nullptr != writer)
            {
                va_list args;
                va_start(args, fmt);
                
                writer(category, file, line, level, fmt, args);

                va_end(args);
            }
            else
            {
                printf("writer is null\n");
            }
        }

        using Writer = std::function<void(const char* /*category*/, const char* /*file*/, int /*line*/, LogLevel, const char* /*fmt*/, va_list /*args*/)>;
        static Writer& GetWriter()
        {
            static Writer writer;
            return writer;
        }
    };
}

#define LOG_FATAL(category, fmt, ...) {\
    zs::Log::Write(#category, __FILE__, __LINE__, zs::LogLevel::LOG_LEVEL_FATAL, fmt, ##__VA_ARGS__); \
    raise(SIGABRT); \
}
#define LOG_ERROR(category, fmt, ...)   zs::Log::Write(#category, __FILE__, __LINE__, zs::LogLevel::LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define LOG_WARN(category, fmt, ...)    zs::Log::Write(#category, __FILE__, __LINE__, zs::LogLevel::LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define LOG_INFO(category, fmt, ...)    zs::Log::Write(#category, __FILE__, __LINE__, zs::LogLevel::LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#ifdef _DEBUG
#define LOG_DEBUG(category, fmt, ...)    zs::Log::Write(#category, __FILE__, __LINE__, zs::LogLevel::LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#elif // _DEBUG
#define LOG_DEBUG(category, fmt, ...)
#endif // _DEBUG

#endif // __ZS_COMMON_LOG_H__
