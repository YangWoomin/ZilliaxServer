
#ifndef __ZS_CACHE_MANAGER_H__
#define __ZS_CACHE_MANAGER_H__

#include    "common/types.h"
#include    "common/log.h"

#include    "cache/cache.h"

#include    <unordered_map>

namespace zs
{
namespace cache
{
    using namespace zs::common;

    class Manager final
    {
    public:
        bool Initialize();
        void Finalize();

        Manager();

    private:
    };
}
}

#endif // __ZS_CACHE_MANAGER_H__
