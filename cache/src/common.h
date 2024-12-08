
#ifndef __ZS_CACHE_COMMON_H__
#define __ZS_CACHE_COMMON_H__

#include    "common/types.h"

#include    <memory>

namespace zs
{
namespace cache
{
    class Worker;
    using WorkerSPtr = std::shared_ptr<Worker>;
}
}

#endif // __ZS_CACHE_COMMON_H__
