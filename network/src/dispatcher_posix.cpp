
#include    "dispatcher.h"

#include    "common/log.h"

#include    "socket.h"

#include    <cstring>

using namespace zs::common;
using namespace zs::network;

#if defined(__GNUC__) || defined(__clang__)

bool Epoll::Initialize()
{
    if (INVALID_FD_VALUE != _epoll)
    {
        return false;
    }

    _epoll = epoll_create1(0);
    if (INVALID_FD_VALUE == _epoll)
    {
        ZS_LOG_ERROR(network, "epoll_create1 failed, err : %d", errno);
        return false;
    }

    // set epoll noti event
    _notiFd = eventfd(0, 0);
    if (INVALID_FD_VALUE == _notiFd)
    {
        ZS_LOG_ERROR(network, "eventfd for noti event failed, err : %d", errno);
        return false;
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = _notiFd;
    if (INVALID_RESULT == epoll_ctl(_epoll, EPOLL_CTL_ADD, _notiFd, &event))
    {
        ZS_LOG_ERROR(network, "epoll_ctl for noti event failed, err : %d", errno);
        return false;
    }

    _events = (epoll_event*)malloc(sizeof(struct epoll_event) * MAX_EVENT_COUNT);

    return true;
}

void Epoll::Close()
{
    if (INVALID_FD_VALUE < _notiFd)
    {
        // send closing noti event
        uint64_t u = CLOSE_SIGNAL;
        write(_notiFd, &u, sizeof(u));
    }
}

void Epoll::Finalize()
{
    if (NULL != _events)
    {
        free(_events);
    }

    if (INVALID_FD_VALUE != _epoll)
    {
        close(_epoll);
        _epoll = INVALID_FD_VALUE;
    }
}

void Epoll::SetOwner(std::size_t workerID)
{
    _workerID = workerID;
}

bool Epoll::Bind(ISocket* sock, BindType bindType, EventType eventType)
{
    if (INVALID_FD_VALUE == _epoll)
    {
        ZS_LOG_ERROR(network, "invalid epoll for bind, socket name : %s",
            sock->GetName());
        return false;
    }
    
    struct epoll_event event;
    std::memset(&event, 0, sizeof(event));
    event.events |= eventType;
    event.data.ptr = sock;
    if (INVALID_RESULT == epoll_ctl(_epoll, bindType, sock->GetSocket(), 
        BindType::UNBIND == bindType ? NULL : &event))
    {
        ZS_LOG_ERROR(network, "epoll_ctl failed for bind, bind type : %d, event type : %d, socket name : %s, socket name : %s, err : %d",
            bindType, eventType, sock->GetName(), errno);
        return false;   
    }

    // if (BindType::BIND == ctx->_bindType)
    // {
    //     ctx->_workerID = _workerID;
    // }

    ZS_LOG_INFO(network, "binding socket on dispatcher succeeded, bind type : %d, event Type : %d, socket name : %s", 
        bindType, eventType, sock->GetName());

    return true;
}

bool Epoll::Dequeue(std::queue<ResultItem>& items)
{
    if (INVALID_FD_VALUE == _epoll)
    {
        ZS_LOG_ERROR(network, "invalid epoll for dequeue");
        return false;
    }

    int32_t count = epoll_wait(_epoll, _events, MAX_EVENT_COUNT, INFINITE);
    if (INVALID_RESULT == count)
    {
        // most cases are to close epoll object from a different thread
        ZS_LOG_ERROR(network, "epoll_wait failed, err : %d", errno);
        return false;
    }

    for (auto i = 0; i < count; ++i)
    {
        ResultItem item;

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

            if (CLOSE_SIGNAL == u)
            {
                // closing noti event
                ZS_LOG_WARN(network, "epoll is being closed");
                item._res._stop = true;
                items.push(item);
                return false;
            }

            continue;
        }

        ISocket* sock = static_cast<ISocket*>(_events[i].data.ptr);
        item._sock = sock;
        
        if (0 != (EPOLLIN & _events[i].events))
        {
            if (SocketType::ACCEPTER == sock->GetType())
            {
                // accept
                AcceptContext* aCtx = new AcceptContext();
                item._iCtx = aCtx;
                aCtx->_len = sizeof(sockaddr_storage);
                aCtx->_sock = accept(sock->GetSocket(), (struct sockaddr*)aCtx->_addr, &aCtx->_len);
                if (INVALID_SOCKET == aCtx->_sock)
                {
                    ZS_LOG_ERROR(network, "accept failed, socket name : %s, err : %d", 
                        sock->GetName(), errno);
                    item._res._release = true;
                }
            }
            else if (SocketType::MESSENGER == sock->GetType())
            {
                // recv
                SendRecvContext* srCtx = new SendRecvContext();
                item._iCtx = srCtx;
                if (Protocol::TCP == sock->GetProtocol())
                {
                    item._res._bytes = recv(sock->GetSocket(), srCtx->_buf, BUFFER_SIZE, 0);
                }
                else if (Protocol::UDP == sock->GetProtocol())
                {
                    srCtx->_addrInfo._len = sizeof(sockaddr_storage);
                    item._res._bytes = recvfrom(sock->GetSocket(), srCtx->_buf, BUFFER_SIZE, 0, (struct sockaddr*)srCtx->_addrInfo._addr, (socklen_t*)&srCtx->_addrInfo._len);
                }
                else
                {
                    ZS_LOG_WARN(network, "unknown protocol, socket name : %s, protocol : %d", 
                        sock->GetName(), sock->GetProtocol());
                    delete srCtx;
                    continue;
                }

                int err = errno;
                if (SOCKET_ERROR == item._res._bytes)
                {
                    if (EAGAIN == err || EWOULDBLOCK == err)
                    {
                        ZS_LOG_WARN(network, "no data for recv, socket name : %s", 
                            sock->GetName());
                        delete srCtx;
                        continue;
                    }

                    ZS_LOG_ERROR(network, "recv failed, socket name : %s, err : %d", 
                        sock->GetName(), err);
                    item._res._release = true;
                }
                else if (0 == item._res._bytes)
                {
                    ZS_LOG_WARN(network, "socket closed, socket name : %s", 
                        sock->GetName());
                    item._res._release = true;
                }
            }
            else
            {
                ZS_LOG_WARN(network, "unknown socket type on EPOLLIN, socket name : %s, socket type : %d", 
                    sock->GetName(), sock->GetType());
                continue;
            }
        }

        if (0 != (EPOLLOUT & _events[i].events))
        {
            if (SocketType::CONNECTOR == sock->GetType())
            {
                item._iCtx = sock->GetContext();
                // int32_t err = 0;
                // socklen_t len = sizeof(err);
                // if (SOCKET_ERROR == getsockopt(sock->GetSocket(), SOL_SOCKET, SO_ERROR, &err, &len))
                // {
                //     ZS_LOG_ERROR(network, "getsockopt for connect failed, err : %d, socket name : %s",
                //         errno, sock->GetName());
                //     item._res._release = true;
                // }
                // if (0 != err)
                // {
                //     ZS_LOG_ERROR(network, "connect failed, socket name : %s, err : %d",
                //         sock->GetName(), errno);
                //     item._res._release = true;
                // }
            }
            else if (SocketType::MESSENGER == sock->GetType())
            {
                // notify sending buffer is available
                SendRecvContext* srCtx = new SendRecvContext();
                item._iCtx = srCtx;
                srCtx->_isRecv = false;
            }
            else
            {
                ZS_LOG_WARN(network, "unknown socket type on EPOLLOUT, socket name : %s, socket type : %d", 
                    sock->GetName(), sock->GetType());
                continue;
            }
        }

        if (_events[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
        {
            ZS_LOG_WARN(network, "epoll error occurred, events : %d, socket name : %s",
                _events[i].events, sock->GetName());
            item._res._release = true;
        }

        items.push(item);
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

void Dispatcher::Stop(std::size_t)
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

void Dispatcher::SetOwner(std::size_t workerID)
{
    if (_epolls.size() <= workerID)
    {
        ZS_LOG_ERROR(network, "invalid worker id for setting owner, worker id : %llu", 
            workerID);
        return;
    }

    _epolls[workerID]->SetOwner(workerID);
}

bool Dispatcher::Bind(std::size_t workerID, ISocket* sock, BindType bindType, EventType eventType)
{
    if (_epolls.size() <= workerID)
    {
        ZS_LOG_ERROR(network, "invalid worker id for bind, worker id : %llu, socket name : %s", 
            workerID, sock->GetName());
        return false;
    }

    if (false == _epolls[workerID]->Bind(sock, bindType, eventType))
    {
        ZS_LOG_ERROR(network, "binding socket failed, worker id : %llu, socket name : %s",
            workerID, sock->GetName());
        return false;
    }

    ZS_LOG_INFO(network, "binding socket on dispatcher succeeded, worker id : %llu, socket name : %s", 
        workerID, sock->GetName());

    return true;
}

bool Dispatcher::Dequeue(std::size_t workerID, std::queue<ResultItem>& items)
{
    if (_epolls.size() <= workerID)
    {
        ZS_LOG_ERROR(network, "invalid worker id for dequeue, worker id : %llu",
            workerID);
        return false;
    }

    return _epolls[workerID]->Dequeue(items);
}

#endif // defined(__GNUC__) || defined(__clang__)
