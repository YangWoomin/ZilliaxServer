
#include    "worker.h"

#include    "dispatcher.h"

using namespace zs::common;
using namespace zs::network;

Worker::Worker(DispatcherSPtr dispatcher, std::size_t workerID)
    : _dispatcher(dispatcher)
    , _workerID(workerID)
{

}

#if defined(__GNUC__) || defined(__clang__)
bool Worker::OwnDispatcher()
{
    if (nullptr != _dispatcher)
    {
        _dispatcher->SetOwner(_workerID);
        return true;
    }

    return false;
}
#endif // defined(__GNUC__) || defined(__clang__)

void Worker::threadMain()
{

}

