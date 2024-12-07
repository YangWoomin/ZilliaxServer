
#include    "network/network.h"

#include    "common/log.h"

#include    "internal_common.h"
#include    "manager.h"

using namespace zs::common;
using namespace zs::network;

static Manager* manager = nullptr;

bool Network::Initialize(Logger::Messenger msgr)
{
    if (nullptr != manager)
    {
        ZS_LOG_ERROR(network, "network module already initialized");
        return false;
    }

    Logger::Messenger& messenger = Logger::GetMessenger();
    messenger = msgr;

#if defined(_WIN64_)
    int         err = 0;
    WSADATA     wsaData;

    err = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (0 != err)
    {
        ZS_LOG_ERROR(network, "WSAStartup failed, err : %d", err);
        return false;
    }
#endif // _WIN64_

    manager = new Manager();

    ZS_LOG_INFO(network, "network module initialized");

    return true;
}

void Network::Finalize()
{
    Network::Stop();

    if (nullptr != manager)
    {
        delete manager;
        manager = nullptr;

        ZS_LOG_INFO(network, "network module finalized");
    }

#if defined(_WIN64_)  
    WSACleanup();
#endif // _WIN64_
}

bool Network::Start(std::size_t dispatcherWorkerCount)
{
    if (nullptr == manager)
    {
        ZS_LOG_ERROR(network, "network module is not initialized");
        return false;
    }

    if (false == manager->Start(dispatcherWorkerCount))
    {
        ZS_LOG_ERROR(network, "network manager start failed");
        return false;
    }

    ZS_LOG_INFO(network, "network module started, dispatcher worker count : %d",
        dispatcherWorkerCount);

    return true;
}

void Network::Stop()
{
    if (nullptr != manager && false == manager->IsStopped())
    {
        manager->Stop();
        ZS_LOG_INFO(network, "network module stopped");
    }
}

bool Network::Bind(IPVer ipVer, Protocol protocol, int32_t port, SocketID& sockID)
{
    if (nullptr == manager)
    {
        ZS_LOG_ERROR(network, "network module is not initialized");
        return false;
    }

    if (IPVer::IP_V4 != ipVer && IPVer::IP_V6 != ipVer)
    {
        ZS_LOG_ERROR(network, "invalid IP version for bind, ip ver : %d",
            ipVer);
        return false;
    }

    if (Protocol::TCP != protocol && Protocol::UDP != protocol)
    {
        ZS_LOG_ERROR(network, "invalid protocol for bind, protocol : %d",
            protocol);
        return false;
    }

    if (AVAILABLE_MINIMUM_PORT > port || AVAILABLE_MAXIMUM_PORT < port)
    {
        ZS_LOG_ERROR(network, "invalid port for bind, available port range : %d ~ %d",
            AVAILABLE_MINIMUM_PORT, AVAILABLE_MAXIMUM_PORT);
        return false;
    }

    if (false == manager->Bind(ipVer, protocol, port, sockID))
    {
        return false;
    }

    return true;
}

bool Network::Listen(SocketID sockID, int32_t backlog, OnConnected onConnected, OnReceived onReceived, OnClosed onClosed)
{
    if (nullptr == manager)
    {
        ZS_LOG_ERROR(network, "network module is not initialized");
        return false;
    }

    if (0 >= backlog || MAX_BACKLOG_SIZE < backlog)
    {
        ZS_LOG_ERROR(network, "invalid backlog for listen, available backlog range : %d ~ %d",
            0, MAX_BACKLOG_SIZE);
        return false;
    }

    if (false == manager->Listen(sockID, backlog, onConnected, onReceived, onClosed))
    {
        return false;
    }

    return true;
}

bool Network::Close(SocketID sockID)
{
    if (nullptr == manager)
    {
        ZS_LOG_ERROR(network, "network module is not initialized");
        return false;
    }

    if (false == manager->Close(sockID))
    {
        return false;
    }

    return true;
}

bool Network::ConnectTCP(IPVer ipVer, std::string host, int32_t port, OnConnected onConnected, OnReceived onReceived, OnClosed onClosed)
{
    if (nullptr == manager)
    {
        ZS_LOG_ERROR(network, "network module is not initialized");
        return false;
    }

    if (IPVer::IP_V4 != ipVer && IPVer::IP_V6 != ipVer)
    {
        ZS_LOG_ERROR(network, "invalid IP version for bind");
        return false;
    }

    if (AVAILABLE_MINIMUM_PORT > port || AVAILABLE_MAXIMUM_PORT < port)
    {
        ZS_LOG_ERROR(network, "invalid port for bind, available port range : %d ~ %d",
            AVAILABLE_MINIMUM_PORT, AVAILABLE_MAXIMUM_PORT);
        return false;
    }

    if (false == manager->Connect(ipVer, Protocol::TCP, host, port, onConnected, onReceived, onClosed))
    {
        return false;
    }

    return true;
}

ConnectionSPtr Network::ConnectUDP(IPVer ipVer, std::string host, int32_t port, OnReceived onReceived)
{
    // implement soon
    return ConnectionSPtr(nullptr);
}
