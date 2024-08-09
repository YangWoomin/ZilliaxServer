
#ifndef __ZS_NETWORK_DISPATCHER_WORKER_H__
#define __ZS_NETWORK_DISPATCHER_WORKER_H__

#include    "common/thread.h"

#include    "internal_common.h"
#include    "manager.h"

namespace zs
{
namespace network
{
    using namespace zs::common;

    class DispatcherWorker final : public Thread<DispatcherWorker>
    {
    public:
        DispatcherWorker(Manager& manager, DispatcherSPtr dispatcher, std::size_t workerID);
        ~DispatcherWorker() = default;

    private:
        void threadMain();
        void handle(IOResult& res);

        DispatcherWorker(const DispatcherWorker&) = delete;
        DispatcherWorker& operator=(const DispatcherWorker&) = delete;

        Manager&        _manager;
        DispatcherSPtr  _dispatcher;
        std::size_t     _workerID;

    friend class Thread<DispatcherWorker>;
    };
}
}

#endif // __ZS_NETWORK_DISPATCHER_WORKER_H__
