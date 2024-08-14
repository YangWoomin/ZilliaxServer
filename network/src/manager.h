
#ifndef __ZS_NETWORK_MANAGER_H__
#define __ZS_NETWORK_MANAGER_H__

#include    "internal_common.h"

#include    <unordered_map>

namespace zs
{
namespace network
{
    class ISocket;

    class Manager final
    {
    public:
        Manager() = default;
        ~Manager() = default;

        bool Start(std::size_t& dispatcherWorkerCount);
        void Stop();
        bool IsStopped();

        bool Bind(IPVer ipVer, Protocol protocol, int32_t port, SocketID& sockID);
        bool Listen(SocketID sockID, int32_t backlog, OnConnectedSPtr onConnected, OnReceivedSPtr onReceived, OnClosedSPtr onClosed);
        bool Close(SocketID sockID);
        bool Connect(IPVer ipVer, Protocol protocol, const std::string& host, int32_t port, OnConnectedSPtr onConnected, OnReceivedSPtr onReceived, OnClosedSPtr onClosed);

        void HandleAccepted(SocketSPtr sock);
        bool HandleConnected(SocketSPtr sock);
        void HandleReceived(SocketSPtr sock);
        void HandleSent(SocketSPtr sock);

        bool InsertSocket(SocketSPtr sock);
        void RemoveSocket(SocketID sockID);
        SocketSPtr GetSocket(SocketID sockID);
        SocketID GenSockID();
#if defined(_POSIX_)
        bool Bind(std::size_t workerID, ISocket* sock, BindType bindType, EventType eventType);
#endif // defined(_POSIX_) 

    private:
        DispatcherSPtr                                      _dispatcher;
        DispatcherWorkers                                   _dispatcherWorkers;
        
        std::mutex                                          _lock;
        std::unordered_map<SocketID, SocketSPtr>            _sockets;
        std::atomic<SocketID>                               _sockIDGen { 0 };

#if defined(_POSIX_)
        std::atomic<std::size_t>                            _workerAllocator { 0 };
#endif // defined(_POSIX_) 

        Manager(const Manager&) = delete;
        Manager& operator=(const Manager&) = delete;
    };
}
}

#endif // __ZS_NETWORK_MANAGER_H__
