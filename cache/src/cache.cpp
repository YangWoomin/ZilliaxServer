
#include    "cache/cache.h"

#include    "common/log.h"

#include    "manager.h"

using namespace zs::common;
using namespace zs::cache;

static Manager* manager = nullptr;

bool Cache::Initialize(Logger::Messenger msgr, const std::string& dsn, int32_t workerCount)
{
    Logger::Messenger& messenger = Logger::GetMessenger();
    messenger = msgr;

    if (nullptr != manager)
    {
        ZS_LOG_ERROR(cache, "cache module already initialized");
        return false;
    }

    manager = new Manager();

    if (false == manager->Initialize(dsn, workerCount))
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

bool Cache::Set(const Script& script, ContextID cid, Keys&& keys, Args&& args, WorkerHash wh, AsyncSet1Callback cb)
{
    if (nullptr == manager)
    {
        ZS_LOG_ERROR(cache, "cache manager not initialized");
        return false;
    }

    return manager->Set(script, cid, std::move(keys), std::move(args), wh, cb);
}

bool Cache::Set(const Script& script, ContextID cid, Keys&& keys, Args&& args, WorkerHash wh, AsyncSet2Callback cb)
{
    if (nullptr == manager)
    {
        ZS_LOG_ERROR(cache, "cache manager not initialized");
        return false;
    }

    return manager->Set(script, cid, std::move(keys), std::move(args), wh, cb);
}

