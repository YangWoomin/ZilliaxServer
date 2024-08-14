
#include    "common/log.h"

#include    "network/network.h"

#include    "network_test/chat_server.h"
#include    "network_test/chat_client.h"

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
    cxxopts::Options options("network_test", "network test option");

    options.add_options()
        ("m,mode", "server | client", cxxopts::value<std::string>()->default_value("server"))
        ("s,server", "domain name or ip of server to connect", cxxopts::value<std::string>()->default_value("localhost"))
        ("p,port", "Server port to listen or connect", cxxopts::value<int>()->default_value("3000"))
        ("h,help", "Print usage")
    ;

    auto result = options.parse(argc, argv);

    if (result.count("help"))
    {
        ZS_LOG_WARN(network_test, "%s", options.help().c_str());
        return 0;
    }

    std::string mode = result["mode"].as<std::string>();
    std::string host = result["server"].as<std::string>();
    int32_t port = result["port"].as<int32_t>();


    // spd async logger
    spdlog::init_thread_pool(8192, 1);
    auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt >();
    auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("../log/network_test.txt", 1024 * 1024 * 10, 10);
    std::vector<spdlog::sink_ptr> sinks{ stdout_sink, rotating_sink };
    auto logger = std::make_shared<spdlog::async_logger>("network_test_" + mode, sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
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

    if ("server" == mode)
    {
        ChatServer(msgr, IPVer::IP_V4, Protocol::TCP, port);
    }
    else if ("client" == mode)
    {
        ChatClient(msgr, IPVer::IP_V4, Protocol::TCP, host, port);
    }

    ZS_LOG_INFO(db_test, "============ network test end ============");

    return 0;
}
