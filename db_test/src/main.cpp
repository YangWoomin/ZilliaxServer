
#include    "common/log.h"
#include    "db/database.h"
#include    "db/operation.h"

#include    "db_test/db_test.h"


#include    "spdlog/spdlog.h"
#include    "spdlog/async.h"
#include    "spdlog/sinks/stdout_color_sinks.h"
#include    "spdlog/sinks/rotating_file_sink.h"

#include    <iostream>


using namespace zs::common;
using namespace zs::db;

int main()
{
    // spd async logger
    spdlog::init_thread_pool(8192, 1);
    auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt >();
    auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("../log/db_test.txt", 1024 * 1024 * 10, 10);
    std::vector<spdlog::sink_ptr> sinks{ stdout_sink, rotating_sink };
    auto logger = std::make_shared<spdlog::async_logger>("db_test", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
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

    ZS_LOG_INFO(db_test, "============ db test start ============");

    Database* db = new Database{};

    if (false == db->Initialize(Config {
        "DRIVER={MySQL ODBC 9.0 ANSI Driver};SERVER=localhost;DATABASE=testdb;USER=test;PASSWORD=test321;OPTION=3;",
        1,     // worker count
        5,      // connection timeout
        5,      // statement timeout
        5,      // connection try timeout
        5,      // connection try count
    }, msgr))
    {
        ZS_LOG_ERROR(db_test, "db initialization failed");
        delete db;
        return 1;
    }

    if (false == db->Start())
    {
        ZS_LOG_ERROR(db_test, "db start failed");
        db->Finalize();
        delete db;
        return 2;
    }

    // SimpleProcCallOperationSPtr so1 = std::make_shared<SimpleProcCallOperation>(100);
    // db->Post(100, so1);

    // SimpleProcCallOperation2SPtr so2 = std::make_shared<SimpleProcCallOperation2>(101);
    // db->Post(101, so2);

    SimpleProcCallOperation3SPtr so3 = std::make_shared<SimpleProcCallOperation3>(102);
    db->Post(100, so3);

    SimpleSelectCallOperationSPtr so4 = std::make_shared<SimpleSelectCallOperation>(103);
    db->Post(100, so4);

    std::this_thread::sleep_for(std::chrono::seconds(5));

    db->Stop();
    db->Finalize();
    delete db;

    ZS_LOG_INFO(db_test, "============ db test end ============");

    return 0;
}
