
#include    "cache/cache.h"

#include    "common/log.h"

#include    "manager.h"

using namespace zs::common;
using namespace zs::cache;

bool Manager::Initialize()
{
    

    ZS_LOG_INFO(cache, "cache manager initialized");

    return true;
}

void Manager::Finalize()
{
    
}

Manager::Manager()
{
    
}
