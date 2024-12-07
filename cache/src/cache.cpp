
#include    "cache/cache.h"

#include    "common/log.h"

#include    "manager.h"

using namespace zs::common;
using namespace zs::cache;

static Manager* manager = nullptr;

bool Cache::Initialize(Logger::Messenger msgr, const std::string& dsn)
{
    Logger::Messenger& messenger = Logger::GetMessenger();
    messenger = msgr;

    if (nullptr != manager)
    {
        ZS_LOG_ERROR(cache, "cache module already initialized");
        return false;
    }

    manager = new Manager();

    if (false == manager->Initialize(dsn))
    {
        ZS_LOG_ERROR(cache, "initializing cache manager failed");
        return false;
    }

    ZS_LOG_INFO(cache, "cache module initialized");

    return true;
}

void Cache::Finalize()
{
    if (nullptr != manager)
    {
        manager->Finalize();
        delete manager;
        manager = nullptr;
    }
}

bool Cache::Set(const std::string& script, const std::vector<std::string>& keys, const std::vector<std::string>& args)
{
    if (nullptr == manager)
    {
        ZS_LOG_ERROR(cache, "cache manager not initialized");
        return false;
    }

    return manager->Set(script, keys, args);
}

