
#ifndef __ZS_CACHE_MANAGER_H__
#define __ZS_CACHE_MANAGER_H__

#include    "common/types.h"
#include    "common/log.h"

#include    "cache/cache.h"
#include    "common.h"

#include    <sw/redis++/redis++.h>
#include    <sw/redis++/async_redis++.h>

#include    <memory>
#include    <vector>

namespace zs
{
namespace cache
{
    using namespace zs::common;
    using namespace sw::redis;

    class Manager final
    {
    public:
        bool Initialize(const std::string& dsn, int32_t workerCount);
        void Finalize();

        bool Run(const Script& script, ContextID cid, Keys&& keys, Args&& args, WorkerHash wh, AsyncSet1Callback cb);
        bool Run(const Script& script, ContextID cid, Keys&& keys, Args&& args, WorkerHash wh, AsyncSet2Callback cb);
        bool Run(const Script& script, ContextID cid, Keys&& keys, Args&& args, ResultSet1& rs);
        bool Run(const Script& script, ContextID cid, Keys&& keys, Args&& args, SimpleResult& rs);

        Manager() = default;
        ~Manager() = default;

    private:
        std::vector<WorkerSPtr>              _workers;
        std::shared_ptr<AsyncRedisCluster>   _arc;
        std::shared_ptr<RedisCluster>        _rc;
    };
}
}

#endif // __ZS_CACHE_MANAGER_H__
