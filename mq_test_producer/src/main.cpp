
#include    "common/log.h"

#include    "network/network.h"

#include    "chat_server.h"

#include    "spdlog/spdlog.h"
#include    "spdlog/async.h"
#include    "spdlog/sinks/stdout_color_sinks.h"
#include    "spdlog/sinks/rotating_file_sink.h"

#include    "cxxopts/cxxopts.hpp"

#include    <iostream>


using namespace zs::common;
using namespace zs::network;

int main(int argc, char** argv)
{
    // parse parameters
    cxxopts::Options options("mq_test_producer", "mq test producer option");
    options.add_options()
        ("p,port", "server port to listen or connect", cxxopts::value<int>()->default_value("3000"))
        ("b,broadcast", "broadcast or unicast echo mode in server", cxxopts::value<bool>()->default_value("false"))
        ("d,dir", "massive test client sample file directory", cxxopts::value<std::string>()->default_value("../../network_test/test_sample_files/"))
        ("h,help", "Print usage");

    auto result = options.parse(argc, argv);
    if (result.count("help"))
    {
        std::cout << options.help() << std::endl;
        return 0;
    }

    int32_t port = result["port"].as<int32_t>();
    bool isBroadcasting = result["broadcast"].as<bool>();
    std::string dir = result["dir"].as<std::string>();

    // spd async logger
    spdlog::init_thread_pool(8192, 1);
    std::string loggerName = "mq_test_producer";
    auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt >();
    auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("../log/" + loggerName + ".txt", 1024 * 1024 * 10, 10);
    std::vector<spdlog::sink_ptr> sinks{ stdout_sink, rotating_sink };
    auto logger = std::make_shared<spdlog::async_logger>(loggerName, sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
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


    ZS_LOG_INFO(network_test, "============ mq test producer start ============");

    ChatServer(msgr, IPVer::IP_V4, Protocol::TCP, port, isBroadcasting, nullptr, nullptr, nullptr);

    ZS_LOG_INFO(network_test, "============ mq test producer end ============");

    return 0;
}
