
#ifndef __ZS_MQ_POLLER_H__
#define __ZS_MQ_POLLER_H__

#include    "common/types.h"
#include    "common/thread.h"

#include    "common.h"

#include    <vector>
#include    <mutex>
#include    <memory>

namespace zs
{
namespace mq
{
    using namespace zs::common;

    class Poller;
    using PollerSPtr = std::shared_ptr<Poller>;

    class Poller final : public Thread<Poller>
    {
    public:
        bool Initialize(int32_t pollingTimeoutMs, int32_t pollingIntervalMs);
        void Finalize();

        void AddPollingBox(PollingBoxWPtr box);

        Poller() = default;
        ~Poller() = default;

    private:
        int32_t                     _timeoutMs = 0;
        int32_t                     _intervalMs = 10;
        std::vector<PollingBoxWPtr> _boxes;
        std::recursive_mutex        _mtx;

        void threadMain();

    friend class Thread<Poller>;
    };
}
}

#endif // __ZS_MQ_POLLER_H__
