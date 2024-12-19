
#include    "chat_server.h"

#include    <iostream>
#include    <thread>
#include    <chrono>
#include    <unordered_map>
#include    <mutex>
#include    <atomic>

#if defined(_WIN64_)
#include    <windows.h>
#elif defined(_POSIX_)
#include    <csignal>
#include    <unistd.h>
#endif // defined(_WIN64_)

using namespace zs::common;
using namespace zs::network;

static std::atomic<bool> run { true };

#if defined(_WIN64_)
static BOOL WINAPI ConsoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT) 
    {
        ZS_LOG_WARN(mq_test_producer, "Ctrl+C detected in ChatServer");
        
        Network::Finalize();

        run = false;

        return TRUE;  // 신호를 처리했음을 알림
    }
    return FALSE;
}
#elif defined(_POSIX_)
static void signalHandler(int signum)
{
    if (signum == SIGINT)
    {
        ZS_LOG_WARN(mq_test_producer, "Interrupt signal (SIGINT) received in ChatServer. Exiting gracefully...");
    }
    else
    {
        ZS_LOG_ERROR(mq_test_producer, "Unhandled interrupt signal %d received in ChatServer", signum);
    }

    Network::Finalize();
    
    run = false;
}
#endif // defined(_WIN64_)

void ChatServer(Logger::Messenger msgr, std::size_t workerCount, IPVer ipVer, Protocol protocol, int32_t port, bool isBroadcasting, OnClientConnected onClientConnected, OnClientClosed onClientClosed, OnMessageReceived onMessageReceived)
{

#if defined(_WIN64_)
    if (SetConsoleCtrlHandler(ConsoleHandler, TRUE))
    {
        ZS_LOG_INFO(mq_test_producer, "Ctrl+C handler installed in ChatServer");
    } 
    else 
    {
        ZS_LOG_ERROR(mq_test_producer, "Failed to install Ctrl+C handler in ChatServer");
        return;
    }
#elif defined(_POSIX_)
    signal(SIGINT, signalHandler);
    ZS_LOG_INFO(mq_test_producer, "Ctrl+C handler installed.");
#endif // defined(_WIN64_)

    if (false == Network::Initialize(msgr))
    {
        ZS_LOG_ERROR(mq_test_producer, "Network::Initialize failed in ChatServer");
        return;
    }
    
    if (false == Network::Start(workerCount))
    {
        ZS_LOG_ERROR(mq_test_producer, "Network::Start failed in ChatServer");
        return;
    }

    SocketID sockID;
    if (false == Network::Bind(ipVer, protocol, port, sockID))
    {
        ZS_LOG_ERROR(mq_test_producer, "Network::Bind failed in ChatServer");
        return;
    }

    ZS_LOG_INFO(mq_test_producer, "Network::Bind succeeded in ChatServer, sock id : %llu", sockID);

    std::mutex mtx;
    std::unordered_map<ConnectionID, ConnectionSPtr> clients;
    std::atomic<std::size_t> totalMsgCount { 0 };
    std::atomic<std::size_t> totalMsgSize { 0 };

    // OnConnected
    OnConnected onConnected = [&mtx, &clients, onClientConnected](ConnectionSPtr conn) {
        if (nullptr != conn)
        {
            ZS_LOG_INFO(mq_test_producer, "a new connection is created in ChatServer, conn id : %llu, peer : %s",
                conn->GetID(), conn->GetPeer());
            
            {
                std::lock_guard<std::mutex> locker(mtx);

                clients.insert(std::make_pair(conn->GetID(), conn));
            }

            if (nullptr != onClientConnected)
            {
                onClientConnected(conn->GetPeer());
            }
        }
        else
        {
            ZS_LOG_ERROR(mq_test_producer, "connetion is nullptr in ChatServer");
        }
    };

    // OnReceived
    OnReceived onReceived = [&mtx, &clients, &totalMsgCount, &totalMsgSize, isBroadcasting, onMessageReceived](ConnectionSPtr conn, const char* buf, std::size_t len) {
        if (nullptr != conn)
        {
            // ConnectionID connID = conn->GetID();
            // ZS_LOG_INFO(mq_test_producer, "data received, conn id : %llu, peer : %s, data : %s", 
            //     connID, conn->GetPeer(), buf);
            
            ++totalMsgCount;

            totalMsgSize += len;

            // broadcasting
            if (true == isBroadcasting)
            {
                std::lock_guard<std::mutex> locker(mtx);
                for (auto it = clients.begin(); clients.end() != it; )
                {
                    if (false == it->second->Send(buf, len))
                    {
                        it = clients.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }
            }
            else
            {
                conn->Send(buf, len);
            }

            if (nullptr != onMessageReceived)
            {
                onMessageReceived(conn->GetPeer(), buf, len);
            }
        }
        else
        {
            ZS_LOG_ERROR(mq_test_producer, "data received from the unknown, data : %s", 
                buf);
        }
    };

    // OnClosed
    OnClosed onClosed = [&mtx, &clients, onClientClosed](ConnectionSPtr conn) {
        if (nullptr != conn)
        {
            ZS_LOG_INFO(mq_test_producer, "the connection closed, conn id : %llu, peer : %s", 
                conn->GetID(), conn->GetPeer());

            ConnectionID connID = conn->GetID();
            {
                std::lock_guard<std::mutex> locker(mtx);

                clients.erase(connID);
            }

            if (nullptr != onClientClosed)
            {
                onClientClosed(conn->GetPeer());
            }
        }
        else
        {
            ZS_LOG_WARN(mq_test_producer, "listener or someone is closed");
        }
    };

    if (false == Network::Listen(sockID, 64, onConnected, onReceived, onClosed))
    {
        ZS_LOG_ERROR(mq_test_producer, "Network::Listen failed");
        return;
    }

    ZS_LOG_INFO(mq_test_producer, "Network::Listen succeeded, sock id : %llu", sockID);

    while (run.load())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    ZS_LOG_INFO(mq_test_producer, "total processed message count : %llu, size : %llu", 
        totalMsgCount.load(), totalMsgSize.load());
}
