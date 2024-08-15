
#include    "dispatcher.h"

#include    "common/log.h"

#include    "socket.h"

using namespace zs::common;
using namespace zs::network;

#if defined(_POSIX_) 

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
        ZS_LOG_ERROR(network, "invalid epoll for bind, socket name : %s, peer : %s",
            sock->GetName(), sock->GetPeer());
        return false;
    }
    
    struct epoll_event event;
    std::memset(&event, 0, sizeof(event));
    if (BindType::BIND == bindType)
    {
        event.events |= eventType;
    }
    else if (BindType::MODIFY == bindType)
    {
        event.events = eventType;
    }
    event.data.ptr = sock;
    if (INVALID_RESULT == epoll_ctl(_epoll, bindType, sock->GetSocket(), 
        BindType::UNBIND == bindType ? NULL : &event))
    {
        ZS_LOG_ERROR(network, "epoll_ctl failed for bind, bind type : %d, event type : %d, socket name : %s, peer : %s, err : %d",
            bindType, eventType, sock->GetName(), sock->GetPeer(), errno);
        return false;   
    }

    if (BindType::BIND == bindType)
    {
        sock->SetWorkerID(_workerID);
    }

    ZS_LOG_INFO(network, "binding socket on dispatcher succeeded, bind type : %d, event Type : %d, socket name : %s, peer : %s", 
        bindType, eventType, sock->GetName(), sock->GetPeer());

    return true;
}

bool Epoll::Dequeue(std::queue<IOResult>& resList)
{
    if (INVALID_FD_VALUE == _epoll)
    {
        ZS_LOG_ERROR(network, "invalid epoll for dequeue");
        return false;
    }

    int32_t count = epoll_wait(_epoll, _events, MAX_EVENT_COUNT, INFINITE);
    if (INVALID_RESULT == count)
    {
        int err = errno;
        if (EINTR == err)
        {
            ZS_LOG_DEBUG(network, "epoll_wait failed by EINTR");
            return true;
        }
        else
        {
            ZS_LOG_ERROR(network, "epoll_wait failed, err : %d", err);
            return false;
        }
    }

    for (auto i = 0; i < count; ++i)
    {
        IOResult res;

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
                ZS_LOG_WARN(network, "epoll is being closed in dispatcher");
                return false;
            }

            continue;
        }

        ISocket* sock = static_cast<ISocket*>(_events[i].data.ptr);
        res._sock = sock->GetSPtr();

        if (_events[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
        {
            ZS_LOG_WARN(network, "epoll error occurred, events : %d, socket name : %s, peer : %s",
                _events[i].events, sock->GetName(), sock->GetPeer());
            
            // unbind this socket from epoll object
            epoll_ctl(_epoll, BindType::UNBIND, sock->GetSocket(), NULL);

            res._release = true;
        }
        
        if (0 != (EPOLLIN & _events[i].events))
        {
            res._eventType = EventType::INBOUND;

            if (SocketType::ACCEPTER == sock->GetType())
            {
                if (Protocol::TCP == sock->GetProtocol())
                {
                    if (false == sock->Accept())
                    {
                        sock->Close();
                        continue;
                    }
                }
                else
                {
                    ZS_LOG_ERROR(network, "unknown protocol in handling accepter, socket name : %s, peer : %s, protocol : %d", 
                        sock->GetName(), sock->GetPeer(), sock->GetProtocol());
                    continue;
                }
            }
            else if (SocketType::MESSENGER == sock->GetType())
            {
                bool later = false;
                if (false == sock->Receive(later))
                {
                    sock->Close();
                    continue;
                }
                if (true == later)
                {
                    if (true != res._release)
                    {
                        continue; // retry later
                    }
                }
            }
            else
            {
                ZS_LOG_WARN(network, "unknown socket type on EPOLLIN, socket name : %s, peer : %s, socket type : %d", 
                    sock->GetName(), sock->GetPeer(), sock->GetType());
                continue;
            }

            resList.push(res);
            res.Reset();
        }

        if (0 != (EPOLLOUT & _events[i].events))
        {
            res._eventType = EventType::OUTBOUND;
            resList.push(res);
            res.Reset();
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
        ZS_LOG_ERROR(network, "invalid worker id for bind, worker id : %llu, socket name : %s, peer : %s", 
            workerID, sock->GetName(), sock->GetPeer());
        return false;
    }

    if (false == _epolls[workerID]->Bind(sock, bindType, eventType))
    {
        ZS_LOG_ERROR(network, "binding socket failed, worker id : %llu, socket name : %s, peer : %s",
            workerID, sock->GetName(), sock->GetPeer());
        return false;
    }

    ZS_LOG_INFO(network, "binding socket on dispatcher succeeded, worker id : %llu, socket name : %s, peer : %s", 
        workerID, sock->GetName(), sock->GetPeer());

    return true;
}

bool Dispatcher::Dequeue(std::size_t workerID, std::queue<IOResult>& resList)
{
    if (_epolls.size() <= workerID)
    {
        ZS_LOG_ERROR(network, "invalid worker id for dequeue, worker id : %llu",
            workerID);
        return false;
    }

    return _epolls[workerID]->Dequeue(resList);
}

#endif // defined(_POSIX_) 
