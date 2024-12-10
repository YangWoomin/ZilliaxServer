
#include    "common/log.h"

#include    "network/network.h"

#include    "chat_server.h"
#include    "msg_manager.h"

#include    "spdlog/spdlog.h"
#include    "spdlog/async.h"
#include    "spdlog/sinks/stdout_color_sinks.h"
#include    "spdlog/sinks/rotating_file_sink.h"

#include    "cxxopts/cxxopts.hpp"

#include    <iostream>
#include    <memory>
#include    <atomic>


using namespace zs::common;
using namespace zs::network;
using namespace zs::cache;

int main(int argc, char** argv)
{
    // parse parameters
    cxxopts::Options options("mq_test_producer", "mq test producer option");
    options.add_options()
        ("p,port", "server port to listen", cxxopts::value<int>()->default_value("3000"))
        ("b,broadcast", "broadcast or unicast echo mode in server", cxxopts::value<bool>()->default_value("false"))
        ("m,mq", "mq servers address delimited by comma", cxxopts::value<std::string>()->default_value("localhost:29092,localhost:39092,localhost:49092"))
        ("d,debug", "mq producer debug mode", cxxopts::value<std::string>()->default_value("generic"))
        ("t,topic", "mq topic", cxxopts::value<std::string>()->default_value("client_message"))
        ("c,cache", "cache server dsn", cxxopts::value<std::string>()->default_value("redis://bitnami@localhost:7000/"))
        ("ttl", "message ttl", cxxopts::value<int>()->default_value("3000"))
        ("h,help", "Print usage");

    auto result = options.parse(argc, argv);
    if (result.count("help"))
    {
        std::cout << options.help() << std::endl;
        return 0;
    }

    // command line setting
    int32_t port = result["port"].as<int32_t>();
    bool isBroadcasting = result["broadcast"].as<bool>();
    std::string cacheAddr = result["cache"].as<std::string>();
    int32_t msgTtl = result["ttl"].as<int32_t>();
    std::string mqAddr = result["mq"].as<std::string>();
    std::string debug = result["debug"].as<std::string>();
    std::string topic = result["topic"].as<std::string>();

    // default setting
    int32_t msgManagerWorkerCount = 2;
    int32_t msgManagerWorkerIntervalMs = 10;
    int32_t cacheWorkerCount = 2;
    int32_t cacheStoredMsgSnTmpListTtlSec = 3000;
    int32_t cacheStoredMsgSnTmpListMaxCount = 1000;
    int32_t cacheSendingMsgLoadCount = 100;
    int32_t mqPollerCount = 2;
    int32_t mqPollingIntervalMs = 10;
    int32_t mqPollingTimeoutMs = 0;

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
    
    CacheConfig cacheConfig = {
        cacheAddr,
        cacheWorkerCount,
        msgTtl,
        cacheStoredMsgSnTmpListTtlSec,
        cacheStoredMsgSnTmpListMaxCount,
        
        cacheSendingMsgLoadCount
    };

    MQConfig mqConfig = {
        mqAddr,
        debug,
        topic,
        mqPollerCount,
        mqPollingIntervalMs,
        mqPollingTimeoutMs,
        // config list
        {
            {"metadata.broker.list", "true"},
        }
    };

    std::shared_ptr<MsgManager> mm = std::make_shared<MsgManager>();
    if (false == mm->Initialize(msgr, msgManagerWorkerCount, msgManagerWorkerIntervalMs, std::move(cacheConfig), std::move(mqConfig)))
    {
        return 0;
    }

    std::weak_ptr<MsgManager> tmpMm = mm;
    auto onClientConnected = [tmpMm](const char* client) {
        std::shared_ptr<MsgManager> mm = tmpMm.lock();
        if (nullptr != mm)
        {
            mm->AddClient(client);
        }
        else
        {
            ZS_LOG_ERROR(mq_test_producer, "missed client connected, client : %s",
                client);
        }
    };
    auto onClientClosed = [tmpMm](const char* client) {
        std::shared_ptr<MsgManager> mm = tmpMm.lock();
        if (nullptr != mm)
        {
            mm->RemoveClient(client);
        }
        else
        {
            ZS_LOG_ERROR(mq_test_producer, "missed client closed, client : %s",
                client);
        }
    };
    auto onMessageReceived = [tmpMm](const char* client, const char* msg, std::size_t len) {
        std::shared_ptr<MsgManager> mm = tmpMm.lock();
        if (nullptr != mm)
        {
            mm->StoreMessage(client, msg, len);
        }
        else
        {
            ZS_LOG_ERROR(mq_test_producer, "missed message, client : %s, msg : %s",
                client, msg);
        }
    };

    ChatServer(msgr, IPVer::IP_V4, Protocol::TCP, port, isBroadcasting, onClientConnected, onClientClosed, onMessageReceived);

    mm.reset();

    ZS_LOG_INFO(mq_test_producer, "============ mq test producer end ============");

    return 0;
}
