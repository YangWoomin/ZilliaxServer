
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

    if (false == bindEvent(_closeNotiFd))
    {
        ZS_LOG_ERROR(network, "binding for close noti event failed");
        return false;
    }

    if (false == bindEvent(_releaseNotiFd))
    {
        ZS_LOG_ERROR(network, "binding for socket release noti event failed");
        return false;
    }

    _events = (epoll_event*)malloc(sizeof(struct epoll_event) * MAX_EVENT_COUNT);

    return true;
}

void Epoll::Close()
{
    if (INVALID_FD_VALUE < _closeNotiFd)
    {
        // send close noti event
        uint64_t sig = CLOSE_SIGNAL;
        write(_closeNotiFd, &sig, sizeof(sig));
    }
}

void Epoll::Finalize()
{
    if (NULL != _events)
    {
        free(_events);
    }

    if (INVALID_FD_VALUE < _closeNotiFd)
    {
        close(_closeNotiFd);
    }

    if (INVALID_FD_VALUE < _releaseNotiFd)
    {
        close(_releaseNotiFd);
    }

    if (INVALID_FD_VALUE != _epoll)
    {
        close(_epoll);
        _epoll = INVALID_FD_VALUE;
    }
}

void Epoll::SetOwner(std::size_t workerID, std::thread::id tid)
{
    _workerID = workerID;
    _tid = tid;
}

bool Epoll::Bind(ISocket* sock, BindType bindType, EventType eventType)
{
    if (INVALID_FD_VALUE == _epoll)
    {
        ZS_LOG_ERROR(network, "invalid epoll for bind, socket name : %s, peer : %s",
            sock->GetName(), sock->GetPeer());
        return false;
    }
    
    // bind the socket
    struct epoll_event event;
    std::memset(&event, 0, sizeof(event));
    if (BindType::BIND == bindType)
    {
        event.events |= eventType;

        sock->SetWorkerID(_workerID);
        sock->SetOwner(_tid);
    }
    else if (BindType::MODIFY == bindType)
    {
        event.events = eventType;
    }
    event.data.ptr = sock;
    if (INVALID_RESULT == epoll_ctl(_epoll, bindType, sock->GetSocket(), 
        BindType::UNBIND == bindType ? NULL : &event))
    {
        ZS_LOG_ERROR(network, "epoll_ctl failed for bind, sock id : %llu, worker id : %llu, bind type : %d, event type : %d, socket name : %s, peer : %s, err : %d",
            sock->GetID(), _workerID, bindType, eventType, sock->GetName(), sock->GetPeer(), errno);
        return false;   
    }

    if (BindType::BIND == bindType)
    {
        sock->SetBound(true);
    }

    // ZS_LOG_INFO(network, "binding socket on dispatcher succeeded, sock id : %llu, worker id : %llu, bind type : %d, event Type : %d, socket name : %s, peer : %s", 
    //     sock->GetID(), _workerID, bindType, eventType, sock->GetName(), sock->GetPeer());

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

        if (_closeNotiFd == _events[i].data.fd)
        {
            // close noti event
            uint64_t data;
            int32_t bytes = read(_events[i].data.fd, &data, sizeof(data));
            if (0 >= bytes)
            {
                ZS_LOG_ERROR(network, "read for close noti event failed, err : %d", 
                    errno);
                continue; // ignore this event at this time
            }

            // this event should be close event
            ZS_LOG_WARN(network, "epoll is being closed in dispatcher");
            return false;
        }

        if (_releaseNotiFd == _events[i].data.fd)
        {
            uint64_t data;
            int32_t bytes = read(_events[i].data.fd, &data, sizeof(data));
            if (0 >= bytes)
            {
                ZS_LOG_ERROR(network, "read for socket release noti event failed, err : %d", 
                    errno);
                continue; // ignore this event at this time
            }

            // socket release noti event
            std::lock_guard<std::mutex> locker(_lock);

            while (false == _releaseSocks.empty())
            {
                res._sock = _releaseSocks.front();
                res._release = true;
                resList.push(res);
                _releaseSocks.pop();
            }

            continue;
        }

        ISocket* sock = static_cast<ISocket*>(_events[i].data.ptr);
        res._sock = sock->GetSPtr();
        if (nullptr == res._sock)
        {
            ZS_LOG_WARN(network, "invalid socket in epoll, events : %d",
                _events[i].events);
            continue;
        }

        if (EPOLLIN & _events[i].events)
        {
            res._eventType = EventType::INBOUND;

            if (SocketType::ACCEPTER == sock->GetType())
            {
                if (Protocol::TCP == sock->GetProtocol())
                {
                    if (false == sock->Accept())
                    {
                        // continue listening
                        //sock->Close();
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
                    //res._release = true;
                    continue;
                }
                if (false == res._release)
                {
                    if (true == later)
                    {
                        ZS_LOG_WARN(network, "receiving is deferred in dispatcher, sock id : %llu, socket name : %s, peer : %s",
                            sock->GetID(), sock->GetName(), sock->GetPeer());
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
        }

        if (EPOLLOUT & _events[i].events)
        {
            res._eventType = EventType::OUTBOUND;
            resList.push(res);
        }

        if ((EPOLLERR | EPOLLHUP | EPOLLRDHUP) & _events[i].events)
        {
            if (SocketType::CONNECTOR == res._sock->GetType())
            {
                // ZS_LOG_WARN(network, "connector is not trying to connect yet, sock id : %llu, socket name : %s, peer : %s",
                //     sock->GetID(), sock->GetName(), sock->GetPeer());
                continue; // trying to connect...
            }

            ZS_LOG_WARN(network, "epoll error occurred, events : %d, sock id : %llu, socket name : %s, peer : %s",
                _events[i].events, sock->GetID(), sock->GetName(), sock->GetPeer());
            
            sock->Close();
            // res._release = true;
            // resList.push(res);
        }
    }

    return true;
}

void Epoll::Release(SocketSPtr sock)
{
    if (INVALID_FD_VALUE < _releaseNotiFd)
    {
        {
            std::lock_guard<std::mutex> locker(_lock);

            _releaseSocks.push(sock);
        }
        
        // send socket release noti event
        uint64_t sig = RELEASE_SIGNAL;
        write(_releaseNotiFd, &sig, sizeof(sig));
    }
}

bool Epoll::bindEvent(int32_t& fd)
{
    fd = eventfd(0, 0);
    if (INVALID_FD_VALUE == fd)
    {
        ZS_LOG_ERROR(network, "eventfd for noti event failed, err : %d", errno);
        return false;
    }

    struct epoll_event event;
    std::memset(&event, 0, sizeof(event));
    event.events = EPOLLIN;
    event.data.fd = fd;
    if (INVALID_RESULT == epoll_ctl(_epoll, EPOLL_CTL_ADD, fd, &event))
    {
        ZS_LOG_ERROR(network, "epoll_ctl for noti event failed, err : %d", errno);
        close(fd);
        fd = INVALID_FD_VALUE;
        return false;
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

void Dispatcher::SetOwner(std::size_t workerID, std::thread::id tid)
{
    if (_epolls.size() <= workerID)
    {
        ZS_LOG_ERROR(network, "invalid worker id for setting owner, worker id : %llu", 
            workerID);
        return;
    }

    _epolls[workerID]->SetOwner(workerID, tid);
}

bool Dispatcher::Bind(std::size_t workerID, ISocket* sock, BindType bindType, EventType eventType)
{
    if (_epolls.size() <= workerID)
    {
        ZS_LOG_ERROR(network, "invalid worker id for bind, worker id : %llu, sock id : %llu, socket name : %s, peer : %s", 
            workerID, sock->GetID(), sock->GetName(), sock->GetPeer());
        return false;
    }

    if (false == _epolls[workerID]->Bind(sock, bindType, eventType))
    {
        ZS_LOG_ERROR(network, "binding socket failed, worker id : %llu, sock id : %llu, socket name : %s, peer : %s",
            workerID, sock->GetID(), sock->GetName(), sock->GetPeer());
        return false;
    }

    // ZS_LOG_INFO(network, "binding socket on dispatcher succeeded, worker id : %llu, socket name : %s, peer : %s", 
    //     workerID, sock->GetName(), sock->GetPeer());

    return true;
}

void Dispatcher::Release(std::size_t workerID, SocketSPtr sock)
{
    if (_epolls.size() <= workerID)
    {
        ZS_LOG_ERROR(network, "invalid worker id for socket release, worker id : %llu",
            workerID);
        return;
    }

    _epolls[workerID]->Release(sock);
}

#endif // defined(_POSIX_) 
