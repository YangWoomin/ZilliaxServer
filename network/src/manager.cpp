
#include    "manager.h"

#include    "common/log.h"

#include    "network/network.h"
#include    "helper.h"
#include    "dispatcher.h"
#include    "dispatcher_worker.h"
#include    "socket.h"

using namespace zs::common;
using namespace zs::network;

bool Manager::Start(std::size_t& dispatcherWorkerCount)
{
    if (nullptr != _dispatcher)
    {
        ZS_LOG_ERROR(network, "network manager already started");
        return false;
    }

    if (false == Helper::Initialize())
    {
        ZS_LOG_ERROR(network, "helper init failed");
        return false;
    }

    if (0 == dispatcherWorkerCount)
    {
        dispatcherWorkerCount = std::thread::hardware_concurrency();
    }
    else if (Network::MAX_WORKER_COUNT < dispatcherWorkerCount)
    {
        ZS_LOG_ERROR(network, "invalid worker count, count : %lu, max : %lu", 
            dispatcherWorkerCount, Network::MAX_WORKER_COUNT);
        return false;
    }

    // dispatcher
    _dispatcher = std::make_shared<Dispatcher>();
    if (false == _dispatcher->Initialize(dispatcherWorkerCount))
    {
        ZS_LOG_ERROR(network, "dispatcher init failed");
        return false;
    }

    // dispatcher workers
    for (std::size_t i = 0; i < dispatcherWorkerCount; ++i)
    {
        DispatcherWorkerSPtr worker = std::make_shared<DispatcherWorker>(*this, _dispatcher, i);
        if (false == worker->Start())
        {
            ZS_LOG_ERROR(network, "worker start failed, worker id : %lu", i);
            return false;
        }

        _dispatcherWorkers.push_back(worker);
    }

    return true;
}

void Manager::Stop()
{
    if (nullptr != _dispatcher)
    {
        for (std::size_t i = 0; i < _dispatcherWorkers.size(); ++i)
        {
            _dispatcherWorkers[i]->Stop();
        }

        _dispatcher->Stop(_dispatcherWorkers.size());

        for (std::size_t i = 0; i < _dispatcherWorkers.size(); ++i)
        {
            _dispatcherWorkers[i]->Join();
        }

        _dispatcher->Finalize();
        _dispatcher.reset();
    }

    for (auto& ele : _sockets)
    {
        ele.second->InvokeOnClosed();
    }
    _sockets.clear();
}

bool Manager::IsStopped()
{
    return nullptr == _dispatcher;
}

bool Manager::Bind(IPVer ipVer, Protocol protocol, int32_t port, SocketID& sockID)
{
    if (nullptr == _dispatcher)
    {
        ZS_LOG_ERROR(network, "network manager not started");
        return false;
    }

    SocketSPtr sock = SocketGenerator::CreateSocket(*this, ++_sockIDGen, SocketType::ACCEPTER, ipVer, protocol);
    if (nullptr == sock)
    {
        ZS_LOG_ERROR(network, "creating listen socket failed, sock id : %llu, ip ver : %d, protocol : %d, port : %d",
            sock->GetID(), ipVer, protocol, port);
        return false;
    }

    if (false == sock->Bind(port))
    {
        ZS_LOG_ERROR(network, "binding listen socket failed, sock id : %llu, ip ver : %d, protocol : %d, port : %d",
            sock->GetID(), ipVer, protocol, port);
        return false;
    }

    if (false == InsertSocket(sock))
    {
        ZS_LOG_ERROR(network, "inserting listen socket failed, sock id : %llu, socket name : %s, ip ver : %d, protocol : %d",
            sock->GetID(), sock->GetName(), ipVer, protocol);
        return false;
    }

    sockID = sock->GetID();

    ZS_LOG_INFO(network, "binding listen socket succeeded, sock id : %llu, socket name : %s, ip ver : %d, protocol : %d",
        sock->GetID(), sock->GetName(), ipVer, protocol);

    return true;
}

bool Manager::Listen(SocketID sockID, int32_t backlog, OnConnected onConnected, OnReceived onReceived, OnClosed onClosed)
{
    if (nullptr == _dispatcher)
    {
        ZS_LOG_ERROR(network, "network manager not started");
        return false;
    }

    SocketSPtr sock = GetSocket(sockID);
    if (nullptr == sock)
    {
        ZS_LOG_ERROR(network, "invalid socket id for listen, socket id : %llu", 
            sockID);
        return false;
    }

    sock->SetCallback(onConnected, onReceived, onClosed);

    if (false == sock->Listen(backlog))
    {
        ZS_LOG_ERROR(network, "listen failed, socket id : %llu, socket name : %s", 
            sockID, sock->GetName());
        RemoveSocket(sock->GetID());
        return false;
    }

    // bind the socket on dispatcher
#if defined(_POSIX_) 
    std::size_t workerID = 0;
    workerID = _workerAllocator++ % _dispatcherWorkers.size();
    if (false == _dispatcher->Bind(workerID, sock.get(), BindType::BIND, EventType::INBOUND))
#elif defined(_WIN64_)
    if (false == _dispatcher->Bind(sock.get()))
#endif // defined(_POSIX_) 
    {
        ZS_LOG_ERROR(network, "binding listen socket on dispatcher failed, sock id : %llu, socket name : %s",
            sock->GetID(), sock->GetName());
        RemoveSocket(sock->GetID()); // not bound yet
        return false;
    }

    ZS_LOG_INFO(network, "listen succeeded, sock id : %llu, socket name : %s",
        sock->GetID(), sock->GetName());

    return true;
}

bool Manager::Close(SocketID sockID)
{
    SocketSPtr sock = GetSocket(sockID);
    if (nullptr == sock)
    {
        ZS_LOG_WARN(network, "invalid socket id for close, socket id : %llu", 
            sockID);
        return false;
    }

    sock->Close();

    return true;
}

bool Manager::Connect(IPVer ipVer, Protocol protocol, const std::string& host, int32_t port, OnConnected onConnected, OnReceived onReceived, OnClosed onClosed)
{
    if (nullptr == _dispatcher)
    {
        ZS_LOG_ERROR(network, "network manager not started");
        return false;
    }

    SocketSPtr sock = SocketGenerator::CreateSocket(*this, ++_sockIDGen, SocketType::CONNECTOR, ipVer, protocol, true/*nonBlocking*/);
    if (nullptr == sock)
    {
        ZS_LOG_ERROR(network, "creating listen socket failed, sock id : %llu, ip ver : %d, protocol : %d, port : %d",
            sock->GetID(), ipVer, protocol, port);
        return false;
    }

    if (false == InsertSocket(sock))
    {
        ZS_LOG_ERROR(network, "inserting connect socket failed, sock id : %llu, ip ver : %d, protocol : %d, host : %s, port : %d",
            sock->GetID(), sock->GetName(), ipVer, protocol, host.c_str(), port);
        return false;
    }

    sock->SetCallback(onConnected, onReceived, onClosed);

    if (false == sock->Bind(0))
    {
        ZS_LOG_ERROR(network, "binding connect socket failed, sock id : %llu, ip ver : %d, protocol : %d, host : %s, port : %d",
            sock->GetID(), sock->GetName(), ipVer, protocol, host.c_str(), port);
        RemoveSocket(sock->GetID()); // not bound yet
        return false;
    }

    // bind the socket on dispatcher
#if defined(_POSIX_) 
    std::size_t workerID = 0;
    workerID = _workerAllocator++ % _dispatcherWorkers.size();
    if (false == _dispatcher->Bind(workerID, sock.get(), BindType::BIND, EventType::OUTBOUND))
#elif defined(_WIN64_)
    if (false == _dispatcher->Bind(sock.get()))
#endif // defined(_WIN64_)
    {
        ZS_LOG_ERROR(network, "binding connect socket on dispatcher failed, sock id : %llu, socket name : %s, ip ver : %d, protocol : %d, host : %s, port : %d",
            sock->GetID(), sock->GetName(), ipVer, protocol, host.c_str(), port);
        RemoveSocket(sock->GetID()); // not bound yet
        return false;
    }

    // test
    // std::string host2;
    // int32_t port2;
    // Helper::GetSockRemoteAddr(sock->GetSocket(), sock->GetIPVer(), host2, port2);
    // if (true == host2.empty())
    // {
    //     ZS_LOG_ERROR(network, "GetSockRemoteAddr, err : %d", errno);
    // }

    if (false == sock->InitConnect(host, port))
    {
        ZS_LOG_ERROR(network, "init connect socket failed, sock id : %llu, socket name : %s, ip ver : %d, protocol : %d",
            sock->GetID(), sock->GetName(), ipVer, protocol);
        sock->Close();
        return false;
    }

    ZS_LOG_INFO(network, "connecting succeeded, sock id : %llu, socket name : %s, ip ver : %d, protocol : %d, host : %s, port : %d",
        sock->GetID(), sock->GetName(), ipVer, protocol, host.c_str(), port);

    return true;
}

void Manager::HandleAccepted(SocketSPtr sock)
{
    // accept the new socket
    SocketSPtr newSock = sock->PostAccept();
    if (nullptr == newSock)
    {
        ZS_LOG_ERROR(network, "post accepting failed, sock id : %llu, socket name : %s",
            sock->GetID(), sock->GetName());
        // sock->Close();
        // return;
    }

    // if in windows reinitiate async accept for the next socket 
    if (false == sock->InitAccept())
    {
        ZS_LOG_ERROR(network, "init accepting failed, sock id : %llu, socket name : %s",
            sock->GetID(), sock->GetName());
        sock->Close(); // close the listen socket
        //return; // continue for the new socket
    }

    // accepting failed
    // continue listening
    if (nullptr == newSock)
    {
        return;
    }

    if (false == InsertSocket(newSock))
    {
        ZS_LOG_ERROR(network, "inserting accepted socket failed, sock id : %llu, socket name : %s, peer : %s",
            newSock->GetID(), newSock->GetName(), newSock->GetPeer());
        //sock->Close();
        return;
    }

    newSock->SetCallback(sock->GetOnConnected(), sock->GetOnReceived(), sock->GetOnClosed());

    // invoke onConnected with the connection as a parameter
    newSock->InvokeOnConnected();

#if defined(_POSIX_)
    // in linux allocate receiving buffer
    if (false == newSock->InitReceive())
    {
        ZS_LOG_ERROR(network, "init receiving failed in handling accepted, sock id : %llu, socket name : %s, peer : %s",
            newSock->GetID(), newSock->GetName(), newSock->GetPeer());
        newSock->Close();
        return;
    }

    // bind the accepted socket to dispatcher for data received event
    std::size_t workerID = 0;
    workerID = _workerAllocator++ % _dispatcherWorkers.size();
    if (false == _dispatcher->Bind(workerID, newSock.get(), BindType::BIND, (EventType)(EventType::INBOUND | EventType::OUTBOUND)))
#elif defined(_WIN64_)
    if (false == _dispatcher->Bind(newSock.get()))
#endif // defined(_POSIX_) 
    {
        ZS_LOG_ERROR(network, "binding messenger socket on dispatcher failed, sock id : %llu, socket name : %s, peer : %s",
            newSock->GetID(), newSock->GetName(), newSock->GetPeer());
        RemoveSocket(newSock->GetID()); // not bound yet
        //return;
    }

#if defined(_WIN64_)
    // in windows initiate async receive on the accepted socket 
    if (false == newSock->InitReceive())
    {
        ZS_LOG_ERROR(network, "init receiving failed in handling accepted, sock id : %llu, socket name : %s, peer : %s",
            newSock->GetID(), newSock->GetName(), newSock->GetPeer());
        newSock->Close();
        return;
    }
#endif // defined(_WIN64_)

    ZS_LOG_INFO(network, "accepting succeeded, sock id : %llu, socket name : %s, peer : %s",
        newSock->GetID(), newSock->GetName(), newSock->GetPeer());

    return;
}

void Manager::HandleConnected(SocketSPtr sock)
{
    bool retry = false;
    if (false == sock->PostConnect(retry))
    {
        ZS_LOG_ERROR(network, "post connect failed, sock id : %llu, socket name : %s",
            sock->GetID(), sock->GetName());
        sock->Close();
        return;
    }
    if (true == retry)
    {
        return;
    }

    // invoke onConnected with the connection as a parameter
    sock->InvokeOnConnected();

    // if windows, initiate async receive on the connected socket
    if (false == sock->InitReceive())
    {
        ZS_LOG_ERROR(network, "init receiving failed in handling connected, sock id : %llu, socket name : %s, peer : %s",
            sock->GetID(), sock->GetName(), sock->GetPeer());
        sock->Close();
        return;
    }

    return;
}

void Manager::HandleReceived(SocketSPtr sock)
{
    // ** in TCP socket **
    // append received data to receiving buffer in connection
    // if the buffer length is enough to make a message then make it
    // pass the message to message buffer in connection
    // invoke onReceived

    // ** UDP **
    // check whether data is received well
    // check whether the socket has a connection
    // if not, create a connection and invoke onConnected
    // invoke onReceived
    
    if (false == sock->PostReceive())
    {
        // most of cases are socket close
        // ZS_LOG_ERROR(network, "post receiving failed, sock id : %llu, socket name : %s, peer : %s",
        //     sock->GetID(), sock->GetName(), sock->GetPeer());
        sock->Close();
        return;
    }

    // if in windows reinitiate async receive on the socket
    if (false == sock->InitReceive())
    {
        ZS_LOG_ERROR(network, "init receiving failed in handling received, sock id : %llu, socket name : %s, peer : %s",
            sock->GetID(), sock->GetName(), sock->GetPeer());
        sock->Close();
        return;
    }

    return;
}

void Manager::HandleSent(SocketSPtr sock)
{
    if (true == sock->PostSend())
    {
        if (false == sock->ContinueSend())
        {
            ZS_LOG_ERROR(network, "continuing sending failed, sock id : %llu, socket name : %s, peer : %s",
                sock->GetID(), sock->GetName(), sock->GetPeer());
            sock->Close();
            return;
        }
    }

    return;
}

bool Manager::InsertSocket(SocketSPtr sock)
{
    std::lock_guard<std::mutex> locker(_lock);

    auto res = _sockets.insert(std::make_pair(sock->GetID(), sock));
    return res.second;
}

void Manager::RemoveSocket(SocketID sockID)
{
    SocketSPtr sock { nullptr };
    {
        std::lock_guard<std::mutex> locker(_lock);

        auto finder = _sockets.find(sockID);
        if (_sockets.end() != finder)
        {
            sock = finder->second;
            _sockets.erase(finder);
        }
    }
    
    if (nullptr != sock)
    {
        sock->InvokeOnClosed();
    }
}

SocketSPtr Manager::GetSocket(SocketID sockID)
{
    std::lock_guard<std::mutex> locker(_lock);

    auto finder = _sockets.find(sockID);
    if (_sockets.end() != finder)
    {
        return finder->second;
    }
    return nullptr;
}

SocketID Manager::GenSockID()
{
    return ++_sockIDGen;
}

#if defined(_POSIX_) 

bool Manager::Bind(std::size_t workerID, ISocket* sock, BindType bindType, EventType eventType)
{
    if (false == _dispatcher->Bind(workerID, sock, bindType, eventType))
    {
        ZS_LOG_ERROR(network, "binding socket on dispatcher failed, sock id : %llu, worker id : %llu, socket name : %s, peer : %s",
            sock->GetID(), workerID, sock->GetName(), sock->GetPeer());
        return false;
    }

    return true;
}

void Manager::ReleaseSocket(std::size_t workerID, SocketSPtr sock)
{
    _dispatcher->Release(workerID, sock);
}

#elif defined(_WIN64_)

bool Manager::ReleaseSocket(SocketSPtr sock)
{
    return _dispatcher->Release(sock);
}

#endif // defined(_POSIX_) 
