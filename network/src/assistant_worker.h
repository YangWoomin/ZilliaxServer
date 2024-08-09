
#ifndef __ZS_NETWORK_ASSISTANT_WORKER_H__
#define __ZS_NETWORK_ASSISTANT_WORKER_H__

#include    "common/thread.h"

#include    "internal_common.h"

namespace zs
{
namespace network
{
    using namespace zs::common;

    class AssistantWorker final : public Thread<AssistantWorker>
    {
    public:
        AssistantWorker(DispatcherSPtr dispatcher, std::size_t workerID);
        ~AssistantWorker() = default;

    private:
        void threadMain();

        AssistantWorker(const AssistantWorker&) = delete;
        AssistantWorker& operator=(const AssistantWorker&) = delete;

        DispatcherSPtr  _dispatcher;
        std::size_t     _workerID;

    friend class Thread<AssistantWorker>;
    };
}
}

#endif // __ZS_NETWORK_ASSISTANT_WORKER_H__
