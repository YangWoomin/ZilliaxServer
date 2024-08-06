
#include    "dispatcher.h"

#include    "common/log.h"

#include    "socket.h"

using namespace zs::common;
using namespace zs::network;

#if defined(_MSVC_)

bool Dispatcher::Initialize(std::size_t workerCount)
{
    if (INVALID_HANDLE_VALUE != _iocp)
    {
        ZS_LOG_ERROR(network, "iocp already created");
        return false;
    }

    _iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, (DWORD)workerCount);
    if (NULL == _iocp)
    {
        ZS_LOG_ERROR(network, "CreateIoCompletionPort failed, err : %lu", 
            GetLastError());
        return false;
    }

    ZS_LOG_INFO(network, "dispatcher initialized");

    return true;
}

void Dispatcher::Stop(std::size_t workerCount)
{
    for (std::size_t i = 0; i < workerCount; ++i)
    {
        if (FALSE == PostQueuedCompletionStatus(_iocp, 0, 0, NULL))
        {
            ZS_LOG_ERROR(network, "PostQueuedCompletionStatus for close failed, err : %lu", 
                GetLastError());
        }
    }

    ZS_LOG_INFO(network, "dispatcher is being closed");
}

void Dispatcher::Finalize()
{
    CloseHandle(_iocp);
    _iocp = INVALID_HANDLE_VALUE;    

    ZS_LOG_INFO(network, "dispatcher finalized");
}

bool Dispatcher::Bind(ISocket* sock)
{
    if (INVALID_HANDLE_VALUE == _iocp)
    {
        ZS_LOG_ERROR(network, "invalid iocp for bind, socket name : %s", 
            sock->GetName());
        return false;
    }

    if (NULL == CreateIoCompletionPort((HANDLE)sock->GetSocket(), _iocp, (ULONG_PTR)sock, 0))
    {
        ZS_LOG_ERROR(network, "CreateIoCompletionPort for bind failed, socket name : %s, err : %lu",
            sock->GetName(), GetLastError());
        return false;
    }

    ZS_LOG_INFO(network, "binding socket on dispatcher succeeded, socket name : %s", 
        sock->GetName());

    return true;
}

bool Dispatcher::Dequeue(std::size_t, std::queue<ResultItem>& items)
{
    if (INVALID_HANDLE_VALUE == _iocp)
    {
        ZS_LOG_ERROR(network, "invalid iocp for dequeue");
        return false;
    }

    ResultItem item;

    item._res._bytes = 0;
    DWORD err = 0;
    ULONG_PTR key = NULL;
    
    if (FALSE == GetQueuedCompletionStatus(_iocp, &(item._res._bytes), &key, (LPOVERLAPPED*)&item._iCtx, INFINITE))
    {
        err = GetLastError();

        if (NULL != key) 
        {
            item._sock = (ISocket*)key;
        }

        // error
        if (NULL == item._iCtx)
        {
            if (ERROR_ABANDONED_WAIT_0 == err)
            {
                ZS_LOG_ERROR(network, "iocp closed, socket name : %s",
                    item._sock ? item._sock->GetName() : "unknown");
                item._res._stop = true;
            }
            else
            {
                ZS_LOG_ERROR(network, "iocp  error, socket name : %s, error : %lu", 
                    item._sock ? item._sock->GetName() : "unknown", err);
                item._res._release = true;
            }

            return false;
        }

        // when using ConnectEx
        if (ERROR_CONNECTION_REFUSED == err)
        {
            ZS_LOG_ERROR(network, "connection refused, socket name : %s",
                item._sock ? item._sock->GetName() : "unknown");
        }
        else
        {
            ZS_LOG_ERROR(network, "GetQueuedCompletionStatus for dequeue failed, socket name : %s, err : %lu",
                item._sock ? item._sock->GetName() : "unknown", err);
        }

        item._res._release = true;
        return false;
    }

    // normal close
    if (NULL == key)
    {
        ZS_LOG_WARN(network, "iocp is being closed fron dequeue");
        item._res._stop = true;
        return false;
    }

    item._sock = (ISocket*)key;

    items.push(item);

    return true;
}

#endif // _MSVC_
