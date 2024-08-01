
#include    "worker.h"

#include    "dispatcher.h"

using namespace zs::common;
using namespace zs::network;

Worker::Worker(DispatcherSPtr dispatcher, std::size_t workerID)
    : _dispatcher(dispatcher)
    , _workerID(workerID)
{

}

#if not defined(_MSVC_)
bool Worker::OwnDispatcher()
{
    if (nullptr != _dispatcher)
    {
        _dispatcher->SetOwner(_workerID, std::this_thread::get_id());
        return true;
    }

    return false;
}
#endif // not _MSVC_

void Worker::threadMain()
{

}

