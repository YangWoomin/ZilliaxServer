
#include    "dispatcher.h"

#include    "common/log.h"

#include    "socket.h"

using namespace zs::common;
using namespace zs::network;

#if defined(_WIN64_)

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

bool Dispatcher::Dequeue(std::size_t, std::queue<IOResult>& resList)
{
    if (INVALID_HANDLE_VALUE == _iocp)
    {
        ZS_LOG_ERROR(network, "invalid iocp for dequeue");
        return false;
    }

    IOResult res;

    DWORD bytes = 0;
    DWORD err = 0;
    ULONG_PTR key = NULL;
    LPOVERLAPPED pOl = NULL;
    
    if (FALSE == GetQueuedCompletionStatus(_iocp, &bytes, &key, (LPOVERLAPPED*)&pOl, INFINITE))
    {
        err = GetLastError();

        ISocket* sock = nullptr;
        if (NULL != key) 
        {
            sock = (ISocket*)key;
            res._sock = sock->GetSPtr();
        }

        // error
        if (NULL == pOl)
        {
            if (ERROR_ABANDONED_WAIT_0 == err)
            {
                ZS_LOG_ERROR(network, "iocp closed, sock id : %llu, socket name : %s, peer : %s",
                    sock ? sock->GetID() : 0,
                    sock ? sock->GetName() : "unknown", 
                    sock ? sock->GetPeer() : "unknown");
                return false;
            }
            else
            {
                ZS_LOG_ERROR(network, "iocp error, sock id : %llu, socket name : %s, peer : %s, error : %lu", 
                    sock ? sock->GetID() : 0,
                    sock ? sock->GetName() : "unknown", 
                    sock ? sock->GetPeer() : "unknown", 
                    err);
            }
            
            res._release = true;
            resList.push(res);

            return true;
        }

        // when using ConnectEx
        if (ERROR_CONNECTION_REFUSED == err)
        {
            ZS_LOG_ERROR(network, "connection refused, sock id : %llu, socket name : %s, peer : %s",
                sock ? sock->GetID() : 0,
                sock ? sock->GetName() : "unknown",
                sock ? sock->GetPeer() : "unknown");
        }
        else
        {
            ZS_LOG_ERROR(network, "GetQueuedCompletionStatus for dequeue failed, sock id : %llu, socket name : %s, peer : %s, err : %lu",
                sock ? sock->GetID() : 0,
                sock ? sock->GetName() : "unknown", 
                sock ? sock->GetPeer() : "unknown", 
                err);
        }

        if (nullptr != sock && 
            (SocketType::ACCEPTER == sock->GetType()
            || SocketType::CONNECTOR == sock->GetType()))
        {
            // keep accepter or connector alive
            resList.push(res);
            return true;
        }

        res._release = true;
        resList.push(res);

        return true;
    }

    // normal close
    if (NULL == key && nullptr == pOl)
    {
        ZS_LOG_WARN(network, "iocp is being closed in dispatcher");
        return false;
    }

    // release socket
    if (NULL == key && nullptr != pOl)
    {
        ReleaseContext* rcCtx = (ReleaseContext*)pOl;
        res._sock = rcCtx->_sock;
        res._release = true;
        resList.push(res);
        delete rcCtx;
        return true;
    }

    ISocket* sock = (ISocket*)key;
    res._sock = sock->GetSPtr();

    SendRecvContext* sCtx = sock->GetSendContext();
    if (nullptr != sCtx && pOl == &(sCtx->_ol))
    {
        res._eventType = EventType::OUTBOUND;
        sCtx->_bytes += bytes;
    }

    SendRecvContext* rCtx = sock->GetRecvContext();
    if (nullptr != rCtx && pOl == &(rCtx->_ol))
    {
        res._eventType = EventType::INBOUND;
        rCtx->_bytes = bytes;
    }

    resList.push(res);

    return true;
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

    sock->SetBound(true);

    // ZS_LOG_INFO(network, "binding socket on dispatcher succeeded, socket name : %s", 
    //     sock->GetName());

    return true;
}

bool Dispatcher::Release(SocketSPtr sock)
{
    ReleaseContext* rcCtx = new ReleaseContext();
    rcCtx->_sock = sock;

    if (FALSE == PostQueuedCompletionStatus(_iocp, 0, 0, &rcCtx->_ol))
    {
        ZS_LOG_ERROR(network, "PostQueuedCompletionStatus for socket release failed, err : %lu", 
            GetLastError());
        delete rcCtx;
        return false;
    }

    return true;
}

#endif // _WIN64_
