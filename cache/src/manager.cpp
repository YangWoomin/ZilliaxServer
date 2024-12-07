
#include    "cache/cache.h"

#include    "common/log.h"

#include    "manager.h"

using namespace zs::common;
using namespace zs::cache;
using namespace sw::redis;

bool Manager::Initialize(const std::string& dsn)
{
    if (nullptr != _arc)
    {
        ZS_LOG_ERROR(cache, "cache manager already initialized");
        return false;
    }

    _arc = std::make_shared<AsyncRedisCluster>(dsn);

    ZS_LOG_INFO(cache, "cache manager initialized");

    return true;
}

void Manager::Finalize()
{
    
}

bool Manager::Set(std::string script, std::vector<std::string> keys, std::vector<std::string> args)
{
    if (nullptr == _arc)
    {
        ZS_LOG_ERROR(cache, "cache manager not initialized");
        return false;
    }

    auto res = _arc->eval<Optional<long long>>(script, keys.begin(), keys.end(), args.begin(), args.end());

    try {
        auto val = res.get();
        if (val)
            ZS_LOG_INFO(cache, "cache eval res : %d",
                *val);
        else
            ZS_LOG_ERROR(cache, "cache eval no res");
    } catch (const Error &err) {
        // handle error
        ZS_LOG_ERROR(cache, "cache eval exception, %s",
            err.what());
    }
    

    return true;
}

Manager::Manager()
{
    
}
