
#include    "common/log.h"

#include    "network/network.h"

#include    "chat_server.h"
#include    "mq_producer.h"

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
        ("s,servers", "mq servers delimited by comma", cxxopts::value<std::string>()->default_value("localhost:29092,localhost:39092,localhost:49092"))
        ("d,debug", "mq producer debug mode", cxxopts::value<std::string>()->default_value("generic"))
        ("t,topic", "mq topic", cxxopts::value<std::string>()->default_value("mq_test_topic"))
        ("h,help", "Print usage");

    auto result = options.parse(argc, argv);
    if (result.count("help"))
    {
        std::cout << options.help() << std::endl;
        return 0;
    }

    int32_t port = result["port"].as<int32_t>();
    bool isBroadcasting = result["broadcast"].as<bool>();
    std::string servers = result["servers"].as<std::string>();
    std::string debug = result["debug"].as<std::string>();
    std::string topic = result["topic"].as<std::string>();

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

    ZS_LOG_INFO(mq_test_producer, "============ mq test producer start ============");

    MQProducer mp;
    if (false == mp.Initialize(msgr, servers, debug))
    {
        return 0;
    }
    
    if (false == mp.CreateProducer(topic))
    {
        return 0;
    }

    uint64_t sn = 0;
    auto onMessageReceived = [&mp, &sn](const char* client, const char* msg, std::size_t len) {
        mp.Produce(client, msg, len, ++sn);
    };

    ChatServer(msgr, IPVer::IP_V4, Protocol::TCP, port, isBroadcasting, nullptr, nullptr, onMessageReceived);

    ZS_LOG_INFO(mq_test_producer, "============ mq test producer end ============");

    return 0;
}