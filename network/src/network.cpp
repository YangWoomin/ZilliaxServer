
#include    "network/network.h"

#include    "common/log.h"

#include    "internal_common.h"
#include    "manager.h"
#include    "dispatcher.h"
#include    "worker.h"

using namespace zs::common;
using namespace zs::network;

Manager* manager = nullptr;

bool Network::Initialize(Logger::Messenger msgr)
{
    if (nullptr != manager)
    {
        ZS_LOG_ERROR(network, "network module already initialized");
        return false;
    }

    Logger::Messenger& messenger = Logger::GetMessenger();
    messenger = msgr;

#if defined(_MSVC_)
    int         err = 0;
    WSADATA     wsaData;

    err = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (0 != err)
    {
        ZS_LOG_ERROR(network, "WSAStartup failed, err : %d", err);
        return false;
    }
#endif // _MSVC_

    manager = new Manager();

    ZS_LOG_INFO(network, "network module initialized");

    return true;
}

void Network::Finalize()
{
    if (nullptr != manager)
    {
        delete manager;
        manager = nullptr;
    }

#if defined(_MSVC_)  
    WSACleanup();
#endif // _MSVC_

    ZS_LOG_INFO(network, "network module finalized");
}

bool Network::Start(std::size_t workerCount)
{
    if (nullptr == manager)
    {
        ZS_LOG_ERROR(network, "network module is not initialized");
        return false;
    }

    if (false == manager->Start(workerCount))
    {
        ZS_LOG_ERROR(network, "network manager start failed");
        return false;
    }

    ZS_LOG_INFO(network, "network module started");

    return true;
}

void Network::Stop()
{
    if (nullptr != manager)
    {
        manager->Stop();
    }

    ZS_LOG_INFO(network, "network module stopped");
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
        ZS_LOG_ERROR(network, "invalid IP version for bind");
        return false;
    }

    if (Protocol::TCP != protocol && Protocol::UDP != protocol)
    {
        ZS_LOG_ERROR(network, "invalid protocol for bind");
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
        ZS_LOG_ERROR(network, "binding failed");
        return false;
    }

    return true;
}

bool Network::Connect(IPVer ipVer, Protocol protocol, int32_t port)
{
    

    return true;
}

void Network::Close(SocketID sID)
{

}


