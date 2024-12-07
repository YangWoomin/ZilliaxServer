
#ifndef __ZS_CACHE_MANAGER_H__
#define __ZS_CACHE_MANAGER_H__

#include    "common/types.h"
#include    "common/log.h"

#include    "cache/cache.h"

#include    <sw/redis++/redis++.h>
#include    <sw/redis++/async_redis++.h>

#include    <memory>

namespace zs
{
namespace cache
{
    using namespace zs::common;
    using namespace sw::redis;

    class Manager final
    {
    public:
        bool Initialize(const std::string& dsn);
        void Finalize();

        bool Set(std::string script, std::vector<std::string> keys, std::vector<std::string> args);

        Manager();

    private:
        std::shared_ptr<AsyncRedisCluster>   _arc;
    };
}
}

#endif // __ZS_CACHE_MANAGER_H__
