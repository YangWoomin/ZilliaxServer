
#include    "common/log.h"

#include    "network/network.h"

#include    "network_test/network_test.h"


#include    "spdlog/spdlog.h"
#include    "spdlog/async.h"
#include    "spdlog/sinks/stdout_color_sinks.h"
#include    "spdlog/sinks/rotating_file_sink.h"

#include    <iostream>


using namespace zs::common;
using namespace zs::network;

int main()
{
    // spd async logger
    spdlog::init_thread_pool(8192, 1);
    auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt >();
    auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("../log/network_test.txt", 1024 * 1024 * 10, 10);
    std::vector<spdlog::sink_ptr> sinks{ stdout_sink, rotating_sink };
    auto logger = std::make_shared<spdlog::async_logger>("network_test", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    spdlog::register_logger(logger);

    Logger::Messenger msgr = [logger](const char* category, const char* file, int line, LogLevel level, const char* fmt, va_list args)
    {
        std::string prefix = std::format("[{0}] ", category);
        //std::string postfix = std::format("     [{0}:{1}]", file, line);
        std::string newFmt = prefix + fmt;// + postfix;
        char buf[1024] = { 0, };
        vsnprintf(buf, sizeof(buf), newFmt.c_str(), args);

        switch (level)
        {
        case LOGLEVEL_DEBUG:
        {
            logger->debug(buf);
            break;
        }
        case LOGLEVEL_INFO:
        {
            logger->info(buf);
            break;
        }
        case LOGLEVEL_WARN:
        {
            logger->warn(buf);
            break;
        }
        case LOGLEVEL_ERROR:
        {
            logger->error(buf);
            break;
        }
        case LOGLEVEL_FATAL:
        {
            logger->critical(buf);
            break;
        }
        default:
        {
            break;
        }
        }
    };

    Logger::Messenger& origin = Logger::GetMessenger();
    origin = msgr;

    ZS_LOG_INFO(db_test, "============ network test start ============");

    Common(msgr);

    Server(3000);

    ZS_LOG_INFO(db_test, "============ network test end ============");

    return 0;
}
