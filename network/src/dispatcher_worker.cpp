
#include    "dispatcher_worker.h"

#include    "common/log.h"

#include    "dispatcher.h"
#include    "socket.h"

using namespace zs::common;
using namespace zs::network;

DispatcherWorker::DispatcherWorker(Manager& manager, DispatcherSPtr dispatcher, std::size_t workerID)
    : _manager(manager)
    , _dispatcher(dispatcher)
    , _workerID(workerID)
{
#if defined(_POSIX_) 
    _dispatcher->SetOwner(_workerID);
#endif // defined(_POSIX_) 
}

void DispatcherWorker::threadMain()
{
    bool isContinue = true;
    std::queue<IOResult> resList;

    while (THREAD_STATUS_RUNNING == getStatus())
    {
        isContinue = _dispatcher->Dequeue(_workerID, resList);
        
        while (false == resList.empty())
        {
            IOResult& res = resList.front();
            handle(res);
            resList.pop();
        }

        if (false == isContinue)
        {
            return; // quit loop
        }
    }
}

void DispatcherWorker::handle(IOResult& res)
{
    if (nullptr == res._sock)
    {
        ZS_LOG_ERROR(network, "invalid socket for handle in dispatcher worker");
        return;
    }

    if (SocketType::ACCEPTER == res._sock->GetType())
    {
        _manager.HandleAccepted(res._sock);
    }
    else if (SocketType::CONNECTOR == res._sock->GetType())
    {
        res._release = !_manager.HandleConnected(res._sock);
    }
    else if (SocketType::MESSENGER == res._sock->GetType())
    {
        if (EventType::INBOUND == res._eventType)
        {
            _manager.HandleReceived(res._sock);
        }
        else if (EventType::OUTBOUND == res._eventType)
        {
            _manager.HandleSent(res._sock);
        }
        else
        {
            ZS_LOG_ERROR(network, "invalid event type of messenger, type : %d, socket name : %s",
                res._eventType, res._sock->GetName());
        }
    }
    else
    {
        ZS_LOG_ERROR(network, "invalid socket type, type : %d, socket name : %s",
            res._sock->GetType(), res._sock->GetName());
    }

    if (true == res._release)
    {
        // release the socket
        _manager.RemoveSocket(res._sock->GetID());

        res._sock->InvokeOnClosed();
    }
}
