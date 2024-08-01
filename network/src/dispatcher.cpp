
#include    "dispatcher.h"

#include    "common/log.h"

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

    _iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, workerCount);
    if (NULL == _iocp)
    {
        ZS_LOG_ERROR(network, "CreateIoCompletionPort failed, err : %lu", GetLastError());
        return false;
    }

    ZS_LOG_INFO(network, "iocp initialized");

    return true;
}

void Dispatcher::Close(std::size_t workerCount)
{
    for (auto i = 0; i < workerCount; ++i)
    {
        if (FALSE == PostQueuedCompletionStatus(_iocp, 0, 0, NULL))
        {
            ZS_LOG_ERROR(network, "PostQueuedCompletionStatus for close failed, err : %lu", GetLastError());
        }
    }

    ZS_LOG_INFO(network, "iocp is being closed by close");
}

void Dispatcher::Finalize()
{
    CloseHandle(_iocp);
    _iocp = INVALID_HANDLE_VALUE;

    ZS_LOG_INFO(network, "iocp finalized");
}

bool Dispatcher::Bind(SocketContext* ctx)
{
    if (nullptr == ctx)
    {
        ZS_LOG_ERROR(network, "invalid socket context for bind");
        return false;
    }

    if (INVALID_HANDLE_VALUE == _iocp)
    {
        ZS_LOG_ERROR(network, "invalid iocp for bind, socket name : %s", ctx->_name);
        return false;
    }

    if (NULL == CreateIoCompletionPort((HANDLE)ctx->_sock, _iocp, (ULONG_PTR)ctx, 0))
    {
        ZS_LOG_ERROR(network, "CreateIoCompletionPort for bind failed, err : %lu, socket name : %s",
            GetLastError(), ctx->_name);
        return false;
    }

    ZS_LOG_INFO(network, "binding socket succeeded, socket name : %s", ctx->_name);

    return true;
}

bool Dispatcher::Dequeue(Result& res, SocketContext*& sCtx, IOContext*& iCtx)
{
    if (INVALID_HANDLE_VALUE == _iocp)
    {
        ZS_LOG_ERROR(network, "invalid iocp for dequeue");
        return false;
    }

    sCtx = NULL;
    iCtx = NULL;

    res._err = ERROR_SUCCESS;
    res._bytes = 0;
    ULONG_PTR key = NULL;
    
    if (FALSE == GetQueuedCompletionStatus(_iocp, &(res._bytes), &key, (LPOVERLAPPED*)&iCtx, INFINITE))
    {
        res._err = GetLastError();

        if (NULL != key) 
        {
            sCtx = (SocketContext*)key;
        }

        // critical error
        if (NULL == iCtx)
        {
            if (ERROR_ABANDONED_WAIT_0 == res._err)
            {
                ZS_LOG_ERROR(network, "iocp closed, socket name : %s",
                    sCtx ? sCtx->_name : "unknown");
            }
            else
            {
                ZS_LOG_ERROR(network, "iocp critical error, error : %lu, socket name : %s", 
                    res._err, sCtx ? sCtx->_name : "unknown");
            }

            // abnormal close
            res._isToBeStopped = true;
            return false;
        }

        // when using ConnectEx
        if (ERROR_CONNECTION_REFUSED == res._err)
        {
            ZS_LOG_ERROR(network, "connection refused, socket name : %s",
                sCtx ? sCtx->_name : "unknown");
        }
        else
        {
            ZS_LOG_ERROR(network, "GetQueuedCompletionStatus for dequeue failed, err : %lu, socket name : %s",
                res._err, sCtx ? sCtx->_name : "unknown");
        }
    }

    // normal close
    if (NULL == key)
    {
        ZS_LOG_WARN(network, "iocp is being closed fron dequeue");
        res._isToBeStopped = true;
        return false;
    }

    sCtx = (SocketContext*)key;

    return true;
}

#else // _MSVC_

bool Epoll::Initialize()
{
    if (-1 != _epoll)
    {
        return false;
    }

    _epoll = epoll_create1(0);
    if (-1 == _epoll)
    {
        ZS_LOG_ERROR(network, "epoll_create1 failed, err : %d", errno);
        return false;
    }

    // set epoll noti event
    _notiFd = eventfd(0, 0);
    if (-1 == _notiFd)
    {
        ZS_LOG_ERROR(network, "eventfd for noti event failed, err : %d", errno);
        return false;
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = _notiFd;
    if (-1 == epoll_ctl(_epoll, EPOLL_CTL_ADD, _notiFd, &event))
    {
        ZS_LOG_ERROR(network, "epoll_ctl for noti event failed, err : %d", errno);
        return false;
    }

    _events = (epoll_event*)malloc(sizeof(struct epoll_event) * MAX_EVENT_COUNT);

    return true;
}

void Epoll::Close()
{
    if (-1 < _notiFd)
    {
        // send closing noti event
        uint64_t u = 0;
        write(_notiFd, &u, sizeof(u));
    }
}

void Epoll::Finalize()
{
    if (NULL != _events)
    {
        free(_events);
    }

    if (-1 != _epoll)
    {
        close(_epoll);
        _epoll = -1;
    }
}

void Epoll::SetOwner(std::thread::id owner)
{
    _owner = owner;
}

bool Epoll::Bind(SocketContext* ctx)
{
    if (nullptr == ctx)
    {
        ZS_LOG_ERROR(network, "invalid ctx for epoll bind");
        return false;
    }

    if (std::this_thread::get_id() != _owner &&
        -1 < _notiFd)
    {
        // if this thread is not this epoll owner
        // send an event for bind
        uint64_t u = reinterpret_cast<uint64_t>(ctx);
        write(_notiFd, &u, sizeof(u));

        ZS_LOG_INFO(network, "sending binding event succeeded, bind type : %d, event type : %d, socket name : %s", 
            ctx->_bindType, ctx->_eventType, ctx->_name);

        return true;
    }
    
    if (std::this_thread::get_id() == _owner &&
        -1 != _epoll)
    {
        // if this thread is this epoll owner
        // handle the event
        struct epoll_event event;
        event.events |= ctx->_eventType;
        event.data.fd = _notiFd;
        if (-1 == epoll_ctl(_epoll, ctx->_bindType, ctx->_sock, 
            BindType::UNBIND == ctx->_bindType ? NULL : &event))
        {
            ZS_LOG_ERROR(network, "epoll_ctl failed for bind, bind type : %d, event type : %d, socket name : %s",
                ctx->_bindType, ctx->_eventType, ctx->_name);
            return false;   
        }

        ZS_LOG_INFO(network, "binding succeeded, bind type : %d, event Type : %d, socket name : %s", 
            ctx->_bindType, ctx->_eventType, ctx->_name);

        return true;
    }

    ZS_LOG_ERROR(network, "something wrong for bind, bind type : %d, event type : %d, socket name : %s", 
        ctx->_bindType, ctx->_eventType, ctx->_name);

    return false;
}

bool Epoll::Dequeue(std::queue<Item>& items)
{
    if (-1 == _epoll)
    {
        ZS_LOG_ERROR(network, "invalid epoll for dequeue");
        return false;
    }

    if (std::this_thread::get_id() != _owner)
    {
        ZS_LOG_FATAL(network, "only epoll owner thread should call this function");
        return false;
    }

    int32_t count = epoll_wait(_epoll, _events, MAX_EVENT_COUNT, -1);
    if (-1 == count)
    {
        ZS_LOG_ERROR(network, "epoll_wait failed, err : %d", errno);
        return false;
    }

    for (auto i = 0; i < count; ++i)
    {
        Item item;

        if (_notiFd == _events[i].data.fd)
        {
            // noti event
            uint64_t u;
            int32_t bytes = read(_events[i].data.fd, &u, sizeof(u));
            if (0 >= bytes)
            {
                ZS_LOG_ERROR(network, "read for noti event failed, err : %d", 
                    errno);
                continue; // ignore this event at this time
            }
            if (0 == u)
            {
                // closing noti event
                ZS_LOG_WARN(network, "epoll is being closed");
                item._res._isToBeStopped = true;
                items.push(item);
                return false;
            }
            else
            {
                // binding event
                SocketContext* sCtx = reinterpret_cast<SocketContext*>(u);
                if (false == this->Bind(sCtx))
                {
                    ZS_LOG_ERROR(network, "binding failed, socket name : %s", 
                        sCtx->_name);
                }
                continue;
            }
        }
        
        if (0 != (EPOLLIN & _events[i].events))
        {
            // IOContext* iCtx = new 
            // std::size_t bytes = recv()
        }

        if (0 != (EPOLLOUT & _events[i].events))
        {

        }
    }

    return true;
}

bool Dispatcher::Initialize(std::size_t workerCount)
{
    if (0 < _epolls.size())
    {
        ZS_LOG_ERROR(network, "epolls already created");
        return false;
    }

    for (std::size_t i = 0; i < workerCount; ++i)
    {
        EpollSPtr epoll = std::make_shared<Epoll>();
        if (false == epoll->Initialize())
        {
            ZS_LOG_ERROR(network, "epoll init failed, worker id : %llu", i);
            return false;
        }
        _epolls.push_back(epoll);
    }

    ZS_LOG_INFO(network, "epolls initialized");

    return true;
}

void Dispatcher::Close(std::size_t)
{
    for (std::size_t i = 0; i < _epolls.size(); ++i)
    {
        _epolls[i]->Close();
    }

    ZS_LOG_INFO(network, "epoll is being closed");
}

void Dispatcher::Finalize()
{
    for (std::size_t i = 0; i < _epolls.size(); ++i)
    {
        _epolls[i]->Finalize();
    }

    _epolls.clear();

    ZS_LOG_INFO(network, "epolls finalized");
}

void Dispatcher::SetOwner(std::size_t workerID, std::thread::id owner)
{
    if (_epolls.size() <= workerID)
    {
        ZS_LOG_ERROR(network, "invalid worker id for setting owner, worker id : %llu", 
            workerID);
        return;
    }

    _epolls[workerID]->SetOwner(owner);
}

bool Dispatcher::Bind(std::size_t workerID, SocketContext* ctx)
{
    if (nullptr == ctx)
    {
        ZS_LOG_ERROR(network, "invalid socket context for bind, worker id : %llu",
            workerID);
        return false;
    }

    if (_epolls.size() <= workerID)
    {
        ZS_LOG_ERROR(network, "invalid worker id for bind, worker id : %llu, socket name : %s", 
            workerID, ctx->_name);
        return false;
    }

    if (false == _epolls[workerID]->Bind(ctx))
    {
        ZS_LOG_ERROR(network, "binding socket failed, worker id : %llu, socket name : %s",
            workerID, ctx->_name);
        return false;
    }

    ZS_LOG_INFO(network, "binding socket succeeded, worker id : %llu, socket name : %s", 
        workerID, ctx->_name);

    return true;
}

bool Dispatcher::Dequeue(std::size_t workerID, std::queue<Item>& items)
{
    if (_epolls.size() <= workerID)
    {
        ZS_LOG_ERROR(network, "invalid worker id for dequeue, worker id : %llu",
            workerID);
        return false;
    }

    return _epolls[workerID]->Dequeue(items);
}

#endif // _MSVC_

