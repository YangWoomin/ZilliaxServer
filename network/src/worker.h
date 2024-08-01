
#ifndef __ZS_NETWORK_WORKER_H__
#define __ZS_NETWORK_WORKER_H__

#include    "common/thread.h"

#include    "internal_common.h"

namespace zs
{
namespace network
{
    using namespace zs::common;

    class Worker final : public Thread<Worker>
    {
    public:
        Worker(DispatcherSPtr dispatcher, std::size_t workerID);
        ~Worker() = default;

#if not defined(_MSVC_)
        bool OwnDispatcher();
#endif // not _MSVC_

    private:
        void threadMain();

        Worker(const Worker&) = delete;
        Worker& operator=(const Worker&) = delete;

        DispatcherSPtr  _dispatcher;
        std::size_t     _workerID;

    friend class Thread<Worker>;
    };
}
}

#endif // __ZS_NETWORK_WORKER_H__
