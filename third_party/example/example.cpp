
#include <iostream>

#define SPDLOG_ACTIVE_LEVEL 0

#include "spdlog/spdlog.h"


// Create stdout/stderr logger object
#include "spdlog/sinks/stdout_color_sinks.h"
void stdout_example()
{
    // create a color multi-threaded logger (category)
    auto console = spdlog::stdout_color_mt("console");
    auto err_logger = spdlog::stderr_color_mt("stderr");
    spdlog::get("console")->info("loggers can be retrieved from a global registry using the spdlog::get(logger_name) 1");
    spdlog::get("stderr")->info("loggers can be retrieved from a global registry using the spdlog::get(logger_name) 2");
}

// Basic file logger
#include "spdlog/sinks/basic_file_sink.h"
void basic_logfile_example()
{
    try
    {
        auto logger = spdlog::basic_logger_mt("basic_logger", "logs/basic-log.txt");
    }
    catch (const spdlog::spdlog_ex& ex)
    {
        std::cout << "Log init failed: " << ex.what() << std::endl;
    }
}

// Rotating files
#include "spdlog/sinks/rotating_file_sink.h"
void rotating_example()
{
    // Create a file rotating logger with 1 MB size max and 3 rotated files
    auto max_size = 1048576;
    auto max_files = 3;
    auto logger = spdlog::rotating_logger_mt("some_logger_name", "logs/rotating.txt", max_size, max_files);
}

// Logger with multi sinks - each with a different format and log level
// create a logger with 2 targets, with different log levels and formats.
// The console will show only warnings or errors, while the file will log all.
void multi_sink_example()
{
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::warn);
    console_sink->set_pattern("[multi_sink_example] [%^%l%$] %v");

    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/multisink.txt", true);
    file_sink->set_level(spdlog::level::trace);

    spdlog::logger logger("multi_sink", {console_sink, file_sink});
    logger.set_level(spdlog::level::debug);
    logger.warn("this should appear in both console and file");
    logger.info("this message should not appear in the console, only in the file");
}

// Asynchronous logging
#include "spdlog/async.h"
void async_example()
{
    // default thread pool settings can be modified *before* creating the async logger:
    // spdlog::init_thread_pool(8192, 1); // queue with 8k items and 1 backing thread.
    auto async_file = spdlog::basic_logger_mt<spdlog::async_factory>("async_file_logger", "logs/async_log.txt");
    // alternatively:
    // auto async_file = spdlog::create_async<spdlog::sinks::basic_file_sink_mt>("async_file_logger", "logs/async_log.txt");   
}

// Asynchronous logger with multi sinks
void multi_sink_example2()
{
    spdlog::init_thread_pool(8192, 1);
    auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt >();
    auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("mylog.txt", 1024 * 1024 * 10, 3);
    std::vector<spdlog::sink_ptr> sinks{ stdout_sink, rotating_sink };
    auto logger = std::make_shared<spdlog::async_logger>("async_logger", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    spdlog::register_logger(logger);
}

#include "log.h"
#include <format>
void test()
{
    zs::Log::Writer& writer = zs::Log::GetWriter();
    writer = [](const char* category, const char* file, int line, zs::LogLevel level, const char* fmt, va_list args) {
        static std::string prefix = std::format("[{0}:{1}] ", file, line);
        std::string newFmt = prefix + fmt;
        char buf[1024] = { 0, };
        vsnprintf(buf, 1024, newFmt.c_str(), args);

        //printf("%s\n", buf);

        spdlog::get("async_logger")->info(buf);
    };
    
    for (size_t i = 0; i < 100; ++i)
    {
        LOG_ERROR(Test, "test %d, %s", 1, "test");
    }

    LOG_FATAL(Test, "test fatal log!!");
}

// signal
void signal_handler(int signal)
{
    if (signal == SIGABRT) {
        spdlog::get("async_logger")->critical("SIGABRT received. Handling abort signal.");

        // Custom cleanup or logging can be added here
        std::exit(signal); // Ensure the program exits
    }
}

int main() 
{
    std::signal(SIGABRT, signal_handler);


    spdlog::info("Welcome to spdlog!");
    spdlog::error("Some error message with arg: {}", 1);
    
    spdlog::warn("Easy padding in numbers like {:08d}", 12);
    spdlog::critical("Support for int: {0:d};  hex: {0:x};  oct: {0:o}; bin: {0:b}", 42);
    spdlog::info("Support for floats {:03.2f}", 1.23456);
    spdlog::info("Positional args are {1} {0}..", "too", "supported");
    spdlog::info("{:<30}", "left aligned");
    
    spdlog::set_level(spdlog::level::debug); // Set global log level to debug
    spdlog::debug("This message should be displayed..");    
    
    // change log pattern
    //spdlog::set_pattern("[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v");
    
    // Compile time log levels
    // Note that this does not change the current log level, it will only
    // remove (depending on SPDLOG_ACTIVE_LEVEL) the call on the release code.
    SPDLOG_TRACE("Some trace message with param {}", 42);
    SPDLOG_DEBUG("Some debug message");

    /*stdout_example();

    basic_logfile_example();
    spdlog::get("basic_logger")->info("loggers can be retrieved from a global registry using the spdlog::get(logger_name)");

    rotating_example();
    for (size_t i = 0; i < 100000; ++i)
    {
        spdlog::get("some_logger_name")->info("loggers can be retrieved from a global registry using the spdlog::get(logger_name)");
    }

    multi_sink_example();

    async_example();*/

    multi_sink_example2();
    
    test();
}