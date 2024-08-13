
#include    "network_test/network_test.h"

#include    "network/network.h"
#include    "network/connection.h"

#include    <iostream>
#include    <thread>
#include    <chrono>

#if defined(_WIN64_)
#include    <windows.h>
#elif defined(_LINUX_)
#include    <csignal>
#include    <unistd.h>
#endif // defined(_WIN64_)

using namespace zs::common;
using namespace zs::network;

#if defined(_WIN64_)
BOOL WINAPI ConsoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT) 
    {
        ZS_LOG_WARN(network_test, "Ctrl+C detected!");
        
        Network::Finalize();

        return TRUE;  // 신호를 처리했음을 알림
    }
    return FALSE;
}
#elif defined(_LINUX_)
void signalHandler(int signum)
{
    if (signum == SIGINT)
    {
        ZS_LOG_WARN(network_test, "Interrupt signal (SIGINT) received. Exiting gracefully...");

        Network::Stop();
        Network::Finalize();
    }
    else
    {
        ZS_LOG_ERROR(network_test, "Unhandled interrupt signal %d received.", signum);
    }
    
    exit(signum);
}
#endif // defined(_WIN64_)

void Common(Logger::Messenger msgr)
{
    if (false == Network::Initialize(msgr))
    {
        ZS_LOG_ERROR(network_test, "Network::Initialize failed");
        return;
    }
}

void Server(int32_t port)
{
#if defined(_WIN64_)
    if (SetConsoleCtrlHandler(ConsoleHandler, TRUE))
    {
        ZS_LOG_INFO(network_test, "Ctrl+C handler installed.");
    } 
    else 
    {
        ZS_LOG_ERROR(network_test, "Failed to install Ctrl+C handler.");
        return;
    }
#elif defined(_LINUX_)
    signal(SIGINT, signalHandler);
    ZS_LOG_INFO(network_test, "Ctrl+C handler installed.");
#endif // defined(_WIN64_)

    if (false == Network::Start())
    {
        ZS_LOG_ERROR(network_test, "Network::Start failed");
        return;
    }

    SocketID sockID;
    if (false == Network::Bind(IPVer::IP_V4, Protocol::TCP, port, sockID))
    {
        ZS_LOG_ERROR(network_test, "Network::Bind failed");
        return;
    }

    ZS_LOG_INFO(network_test, "Network::Bind succeeded, sock id : %llu", sockID);

    // OnConnected
    OnConnectedSPtr onConnected = std::make_shared<OnConnected>([](ConnectionWPtr connection) {
        ConnectionSPtr conn = connection.lock();
        if (nullptr != conn)
        {
            ZS_LOG_INFO(network_test, "a new connection created, conn id : %llu, peer : %s", 
                conn->GetID(), conn->GetPeer());
        }
        else
        {
            ZS_LOG_ERROR(network_test, "connecting failed");
        }
    });

    // OnReceived
    OnReceivedSPtr onReceived = std::make_shared<OnReceived>([](ConnectionWPtr connection, const char* buf, std::size_t len) {
        ConnectionSPtr conn = connection.lock();
        std::string data (buf, len);
        if (nullptr != conn)
        {
            ZS_LOG_INFO(network_test, "data is received, conn id : %llu, peer : %s, data : %s", 
                conn->GetID(), conn->GetPeer(), data.c_str());
        }
        else
        {
            ZS_LOG_ERROR(network_test, "receiving data failed, data : %s", data.c_str());
        }
    });

    // OnClosed
    OnClosedSPtr onClosed = std::make_shared<OnClosed>([](ConnectionWPtr connection) {
        ConnectionSPtr conn = connection.lock();
        if (nullptr != conn)
        {
            ZS_LOG_INFO(network_test, "the connection closed, conn id : %llu, peer : %s", 
                conn->GetID(), conn->GetPeer());
        }
        else
        {
            ZS_LOG_ERROR(network_test, "someone closed");
        }
    });

    if (false == Network::Listen(sockID, 64, onConnected, onReceived, onClosed))
    {
        ZS_LOG_ERROR(network_test, "Network::Listen failed");
        return;
    }

    ZS_LOG_INFO(network_test, "Network::Listen succeeded, sock id : %llu", sockID);

    std::this_thread::sleep_for(std::chrono::seconds(300));

    Network::Finalize();
}
