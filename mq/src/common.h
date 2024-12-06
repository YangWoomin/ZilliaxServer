
#ifndef __ZS_MQ_COMMON_H__
#define __ZS_MQ_COMMON_H__

#include    "common/types.h"

#include    <memory>

namespace zs
{
namespace mq
{
    class PollingBox
    {
    public:
        virtual int32_t Poll(int32_t timeoutMs) = 0;
    };

    using PollingBoxSPtr = std::shared_ptr<PollingBox>;
    using PollingBoxWPtr = std::weak_ptr<PollingBox>;
}
}

#endif // __ZS_MQ_COMMON_H__
