
#include    "manager.h"

#include    "common/log.h"

#include    "network/network.h"
#include    "helper.h"
#include    "dispatcher.h"
#include    "worker.h"
#include    "socket.h"

#include    <cstring>

using namespace zs::common;
using namespace zs::network;

bool Manager::Start(std::size_t asyncSendWorkerCount, std::size_t& dispatcherWorkerCount)
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
        WorkerSPtr worker = std::make_shared<Worker>(_dispatcher, i);
        if (false == worker->Start())
        {
            ZS_LOG_ERROR(network, "worker start failed, worker id : %lu", i);
            return false;
        }

#if defined(__GNUC__) || defined(__clang__)
        if (false == worker->OwnDispatcher())
        {
            ZS_LOG_ERROR(network, "worker failed to own dispatcher, worker id : %lu", i);
            return false;
        }
#endif // defined(__GNUC__) || defined(__clang__)

        _dispatcherWorkers.push_back(worker);
    }

    // async send workers

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
    }
}

bool Manager::Bind(IPVer ipVer, Protocol protocol, int32_t port, SocketID& sockID)
{
    if (nullptr == _dispatcher)
    {
        ZS_LOG_ERROR(network, "network manager not started");
        return false;
    }

    SocketSPtr sock = SocketGenerator::CreateSocket(++_sockIDGen, SocketType::ACCEPTER, ipVer, protocol);
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

    // bind the socket on dispatcher
#if defined(__GNUC__) || defined(__clang__)
    std::size_t workerID = 0;
    workerID = _workerAllocator++ % _dispatcherWorkers.size();
    if (false == _dispatcher->Bind(workerID, sock.get(), BindType::BIND, EventType::INBOUND))
#elif defined(_MSVC_)
    if (false == _dispatcher->Bind(sock.get()))
#endif // defined(__GNUC__) || defined(__clang__)
    {
        ZS_LOG_ERROR(network, "binding listen socket on dispatcher failed, sock id : %llu, socket name : %s, ip ver : %d, protocol : %d",
            sock->GetID(), sock->GetName(), ipVer, protocol);
        RemoveSocket(sock->GetID()); // not bound yet
        return false;
    }

    sockID = sock->GetID();

    ZS_LOG_INFO(network, "binding listen socket succeeded, sock id : %llu, socket name : %s, ip ver : %d, protocol : %d",
        sock->GetID(), sock->GetName(), ipVer, protocol);

    return true;
}

bool Manager::Listen(SocketID sockID, int32_t backlog, OnConnectedSPtr onConnected, OnReceivedSPtr onReceived)
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

    if (false == sock->Listen(backlog, onConnected, onReceived))
    {
        ZS_LOG_ERROR(network, "listen failed, socket id : %llu, socket name : %s", 
            sockID, sock->GetName());
        sock->Close();
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

bool Manager::Connect(IPVer ipVer, Protocol protocol, const std::string& host, int32_t port, OnConnectedSPtr onConnected, OnReceivedSPtr onReceived)
{
    if (nullptr == _dispatcher)
    {
        ZS_LOG_ERROR(network, "network manager not started");
        return false;
    }

    SocketSPtr sock = SocketGenerator::CreateSocket(++_sockIDGen, SocketType::CONNECTOR, ipVer, protocol, true/*nonBlocking*/);
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

    if (false == sock->Bind(0))
    {
        ZS_LOG_ERROR(network, "binding connect socket failed, sock id : %llu, ip ver : %d, protocol : %d, host : %s, port : %d",
            sock->GetID(), sock->GetName(), ipVer, protocol, host.c_str(), port);
        RemoveSocket(sock->GetID()); // not bound yet
        return false;
    }

    // bind the socket on dispatcher
#if defined(__GNUC__) || defined(__clang__)
    std::size_t workerID = 0;
    workerID = _workerAllocator++ % _dispatcherWorkers.size();
    if (false == _dispatcher->Bind(workerID, sock.get(), BindType::BIND, EventType::OUTBOUND))
#elif defined(_MSVC_)
    if (false == _dispatcher->Bind(sock.get()))
#endif // defined(__GNUC__) || defined(__clang__)
    {
        ZS_LOG_ERROR(network, "binding connect socket on dispatcher failed, sock id : %llu, socket name : %s, ip ver : %d, protocol : %d, host : %s, port : %d",
            sock->GetID(), sock->GetName(), ipVer, protocol, host.c_str(), port);
        RemoveSocket(sock->GetID()); // not bound yet
        return false;
    }

    // connect
    if (false == sock->Connect(host, port, onConnected, onReceived))
    {
        ZS_LOG_ERROR(network, "inserting connect socket failed, sock id : %llu, ip ver : %d, protocol : %d, host : %s, port : %d",
            sock->GetID(), sock->GetName(), ipVer, protocol);
        sock->Close();
        return false;
    }

    ZS_LOG_INFO(network, "connecting succeeded, sock id : %llu, ip ver : %d, protocol : %d, host : %s, port : %d",
        sock->GetID(), sock->GetName(), ipVer, protocol, host.c_str(), port);

    return true;
}

bool Manager::InsertSocket(SocketSPtr sockBase)
{
    std::lock_guard<std::mutex> locker(_lock);

    auto res = _sockets.insert(std::make_pair(sockBase->GetID(), sockBase));
    return res.second;
}

void Manager::RemoveSocket(SocketID sockID)
{
    std::lock_guard<std::mutex> locker(_lock);

    _sockets.erase(sockID);
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
