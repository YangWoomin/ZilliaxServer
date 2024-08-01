
#ifndef __ZS_NETWORK_MANAGER_H__
#define __ZS_NETWORK_MANAGER_H__

#include    "network/common.h"
#include    "internal_common.h"

#include    <unordered_map>
#include    <mutex>

#if not defined(_MSVC_)
#include    <atomic>
#endif // not _MSVC_

namespace zs
{
namespace network
{
    class Manager final
    {
    public:
        Manager() = default;
        ~Manager() = default;

        // thread unsafe functions
        bool Start(std::size_t workerCount);
        void Stop();

        // thread safe functions
        bool Bind(IPVer ipVer, Protocol protocol, int32_t port, SocketID& sockID);

    private:
        bool insertSocketContext(SocketContext* sCtx, SocketID& sockID);
        void removeSocketContext(SocketID sockID);
    
    public:
        static int32_t GetIPVerValue(IPVer ipVer);
        static int32_t GetProtocolValue(Protocol protocol);
        static int32_t GetSocketTypeValue(Protocol protocol);
        static void GetSockAddrIn(int32_t port, sockaddr_in& addr, std::string& host);
        static void GetSockAddrIn(int32_t port, sockaddr_in6& addr, std::string& host);

    private:
        DispatcherSPtr                                  _dispatcher;
        Workers                                         _workers;
        std::mutex                                      _lock;
        std::unordered_map<SocketID, SocketContext*>    _listeners;
        SocketID                                        _sockID = 0;

#if not defined(_MSVC_)
        std::atomic<std::size_t>        _allocator { 0 };
#endif // not _MSVC_
    };
}
}

#endif // __ZS_NETWORK_MANAGER_H__
