
#include    "manager.h"

#include    "common/log.h"

#include    "network/network.h"
#include    "dispatcher.h"
#include    "worker.h"

#include    <cstring>

using namespace zs::common;
using namespace zs::network;

bool Manager::Start(std::size_t workerCount)
{
    if (nullptr != _dispatcher)
    {
        ZS_LOG_ERROR(network, "network manager already started");
        return false;
    }

    _dispatcher = std::make_shared<Dispatcher>();
    if (false == _dispatcher->Initialize(workerCount))
    {
        ZS_LOG_ERROR(network, "dispatcher init failed");
        return false;
    }

    if (0 == workerCount)
    {
        workerCount = std::thread::hardware_concurrency();
    }
    else if (Network::MAX_WORKER_COUNT < workerCount)
    {
        ZS_LOG_ERROR(network, "invalid worker count, count : %lu, max : %lu", 
            workerCount, Network::MAX_WORKER_COUNT);
        return false;
    }

    for (std::size_t i = 0; i < workerCount; ++i)
    {
        WorkerSPtr worker = std::make_shared<Worker>(_dispatcher, i);
        if (false == worker->Start())
        {
            ZS_LOG_ERROR(network, "worker start failed, worker id : %lu", i);
            return false;
        }

#if not defined(_MSVC_)
        if (false == worker->OwnDispatcher())
        {
            ZS_LOG_ERROR(network, "worker failed to own dispatcher, worker id : %lu", i);
            return false;
        }
#endif // not _MSVC_

        _workers.push_back(worker);
    }

    return true;
}

void Manager::Stop()
{
    if (nullptr != _dispatcher)
    {
        for (std::size_t i = 0; i < _workers.size(); ++i)
        {
            _workers[i]->Stop();
        }

        _dispatcher->Close(_workers.size());

        for (std::size_t i = 0; i < _workers.size(); ++i)
        {
            _workers[i]->Join();
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

    // create a socket
#if defined(_MSVC_)
    Socket sock = WSASocket(
        Manager::GetIPVerValue(ipVer), 
        Manager::GetSocketTypeValue(protocol), 
        Manager::GetProtocolValue(protocol), 
        NULL, 0, WSA_FLAG_OVERLAPPED);
#else // _MSVC_
    Socket sock = socket(
        Manager::GetIPVerValue(ipVer), 
        Manager::GetSocketTypeValue(protocol), 
        Manager::GetProtocolValue(protocol));
#endif // _MSVC_
    if (INVALID_SOCKET == sock)
    {
        ZS_LOG_ERROR(network, "socket failed, ip ver : %d, protocol : %d, err : %d", 
            ipVer, protocol, errno);
        return false;
    }

    // set socket reuse option
    int reuse = 1;
    if (SOCKET_ERROR == setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)))
    {
        ZS_LOG_ERROR(network, "setsockopt for reuse failed, ip ver : %d, protocol : %d, err : %d", 
            ipVer, protocol, errno);
        CloseSocket(sock);
        return false;
    }

    // bind the socket
    std::string name;
    void* addr = nullptr;
    int len = 0;
    sockaddr_in addrV4;
    sockaddr_in6 addrV6;
    if (IPVer::IP_V4 == ipVer)
    {
        Manager::GetSockAddrIn(port, addrV4, name);
        addr = &addrV4;
        len = sizeof(addrV4);
    }
    else // IPVer::IP_V6 == ipVer
    {
        Manager::GetSockAddrIn(port, addrV6, name);
        addr = &addrV6;
        len = sizeof(addrV6);
    }
    if (SOCKET_ERROR == bind(sock, (struct sockaddr*)addr, len))
    {
        ZS_LOG_ERROR(network, "socket binding failed, ip ver : %d, protocol : %d, err : %d", 
            ipVer, protocol, errno);
        CloseSocket(sock);
        return false;
    }

    // add socket context to listeners
    SocketContext* sCtx = new SocketContext();
    if (false == insertSocketContext(sCtx, sockID))
    {
        ZS_LOG_ERROR(network, "inserting socket context failed, ip ver : %d, protocol : %d",
            ipVer, protocol);
        CloseSocket(sock);
        delete sCtx;
        return false;
    }

    // bind the socket on dispatcher
    name += (":" + std::to_string(port));
    std::strncpy(sCtx->_name, name.c_str(), sizeof(sCtx->_name));
    sCtx->_sock = sock;
    sCtx->_sockType = SocketType::ACCEPTER;
#if not defined(_MSVC_)
    sCtx->_bindType = BindType::BIND;
    sCtx->_eventType = EventType::INBOUND;
    std::size_t workerID = _allocator++ % _workers.size();
    if (false == _dispatcher->Bind(workerID, sCtx))
#else // not _MSVC_
    if (false == _dispatcher->Bind(sCtx))
#endif // _MSVC_
    {
        ZS_LOG_ERROR(network, "socket binding on dispatcher failed, ip ver : %d, protocol : %d",
            ipVer, protocol);
        removeSocketContext(sockID);
        CloseSocket(sock);
        delete sCtx;
        return false;
    }

    return true;
}

bool Manager::insertSocketContext(SocketContext* sCtx, SocketID& sockID)
{
    std::lock_guard<std::mutex> locker(_lock);

    auto res = _listeners.insert(std::make_pair(++_sockID, sCtx));
    if (false == res.second)
    {
        return false;
    }
    else
    {
        sockID = res.first->first;
        return true;
    }
}

void Manager::removeSocketContext(SocketID sockID)
{
    std::lock_guard<std::mutex> locker(_lock);

    _listeners.erase(sockID);
}

int32_t Manager::GetIPVerValue(IPVer ipVer)
{
    switch (ipVer)
    {
    case IPVer::IP_V4:
    {
        return AF_INET;
    }
    case IPVer::IP_V6:
    {
        return AF_INET6;
    }
    default:
    {
        ZS_LOG_FATAL(network, "invalid ip version, ip version : %d", ipVer);
        return AF_INET;
    }
    }
}

int32_t Manager::GetProtocolValue(Protocol protocol)
{
    switch (protocol)
    {
    case Protocol::TCP:
    {
        return IPPROTO_TCP;
    }
    case Protocol::UDP:
    {
        return IPPROTO_UDP;
    }
    default:
    {
        ZS_LOG_FATAL(network, "invalid protocol, protocol : %d", protocol);
        return IPPROTO_TCP;
    }
    }
}

int32_t Manager::GetSocketTypeValue(Protocol protocol)
{
    switch (protocol)
    {
    case Protocol::TCP:
    {
        return SOCK_STREAM;
    }
    case Protocol::UDP:
    {
        return SOCK_DGRAM;
    }
    default:
    {
        ZS_LOG_FATAL(network, "invalid protocol, protocol : %d", protocol);
        return SOCK_STREAM;
    }
    }
}

void Manager::GetSockAddrIn(int32_t port, sockaddr_in& addr, std::string& host)
{
    std::memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    host.resize(INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &addr, host.data(), INET_ADDRSTRLEN);
}

void Manager::GetSockAddrIn(int32_t port, sockaddr_in6& addr, std::string& host)
{
    memset(&addr, 0, sizeof(addr));

    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);
    addr.sin6_addr = in6addr_any;

    host.resize(INET6_ADDRSTRLEN);
    inet_ntop(AF_INET6, &addr, host.data(), INET6_ADDRSTRLEN);
}
