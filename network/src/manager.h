
#ifndef __ZS_NETWORK_MANAGER_H__
#define __ZS_NETWORK_MANAGER_H__

#include    "internal_common.h"

#include    <unordered_map>
#include    <mutex>
#include    <atomic>

namespace zs
{
namespace network
{
    class Manager final
    {
    public:
        Manager() = default;
        ~Manager() = default;

        bool Start(std::size_t asyncSendWorkerCount, std::size_t& dispatcherWorkerCount);
        void Stop();

        bool Bind(IPVer ipVer, Protocol protocol, int32_t port, SocketID& sockID);
        bool Listen(SocketID sockID, int32_t backlog, OnConnectedSPtr onConnected, OnReceivedSPtr onReceived);
        bool Close(SocketID sockID);
        bool Connect(IPVer ipVer, Protocol protocol, const std::string& host, int32_t port, OnConnectedSPtr onConnected, OnReceivedSPtr onReceived);

        bool InsertSocket(SocketSPtr sockBase);
        void RemoveSocket(SocketID sockID);
        SocketSPtr GetSocket(SocketID sockID);

    private:
        DispatcherSPtr                                  _dispatcher;
        Workers                                         _dispatcherWorkers;
        
        std::mutex                                      _lock;
        std::unordered_map<SocketID, SocketSPtr>        _sockets;
        std::atomic<SocketID>                           _sockIDGen = 0;

#if defined(__GNUC__) || defined(__clang__)
        std::atomic<std::size_t>                        _workerAllocator { 0 };
#endif // defined(__GNUC__) || defined(__clang__)

        Manager(const Manager&) = delete;
        Manager& operator=(const Manager&) = delete;
    };
}
}

#endif // __ZS_NETWORK_MANAGER_H__
