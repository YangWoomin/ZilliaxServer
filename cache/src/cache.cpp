
#include    "cache/cache.h"

#include    "common/log.h"

#include    <sw/redis++/redis++.h>
#include    <sw/redis++/async_redis++.h>

using namespace zs::common;
using namespace zs::cache;
using namespace sw::redis;

bool Cache::Initialize(Logger::Messenger msgr)
{

    auto redis = Redis("tcp://127.0.0.1:6379");
    
    
    ConnectionOptions opts;
    opts.host = "127.0.0.1";
    opts.port = 6379;

    ConnectionPoolOptions pool_opts;
    pool_opts.size = 3;

    auto async_redis = AsyncRedis(opts, pool_opts);

    ZS_LOG_INFO(cache, "mq module initialized");

    return true;
}

void Cache::Finalize()
{
    
}

