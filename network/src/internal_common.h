
#ifndef __ZS_NETWORK_INTERNAL_COMMON_H__
#define __ZS_NETWORK_INTERNAL_COMMON_H__

#if defined(_WIN64_)

#include    <WinSock2.h>
#include    <mswsock.h>
#include    <Windows.h>
#include    <ws2def.h>
#include    <ws2ipdef.h>
#include    <ws2tcpip.h>

#elif defined(_POSIX_) 

#include    <unistd.h>
#include    <errno.h>
#include    <sys/types.h>
#include    <sys/epoll.h>
#include    <sys/eventfd.h>
#include    <sys/socket.h>
#include    <arpa/inet.h>
#include    <netinet/in.h>
#include    <fcntl.h>
#include    <netdb.h>

#include    <limits>

#endif // _WIN64_


#include    "common/lock.h"
#include    "network/common.h"

#include    <vector>
#include    <string>
#include    <thread>
#include    <atomic>
#include    <cstring>
#include    <mutex>
#include    <queue>
#include    <deque>

namespace zs
{
namespace network
{
    class DispatcherWorker;
    using DispatcherWorkerSPtr = std::shared_ptr<DispatcherWorker>;
    using DispatcherWorkers = std::vector<DispatcherWorkerSPtr>;

    class Dispatcher;
    using DispatcherSPtr = std::shared_ptr<Dispatcher>;

    static const std::size_t SOCKET_NAME_SIZE = 128;

#if defined(_POSIX_) 
    enum BindType : int32_t
    {
        BIND            = EPOLL_CTL_ADD,
        UNBIND          = EPOLL_CTL_DEL,
        MODIFY          = EPOLL_CTL_MOD,
    };

    enum EventType : int32_t
    {
        INBOUND         = EPOLLIN,      // triggered when some data is received or a remote client is accepted
        OUTBOUND        = EPOLLOUT,     // triggered when socket affords to get sending data or a remote client is connected
    };

#define NO_ERROR                0
#define INVALID_RESULT          -1
#define INVALID_FD_VALUE        -1
#define INVALID_SOCKET          -1
#define SOCKET_ERROR            -1
#define CONN_REFUSED            ECONNREFUSED 
#define CONN_TIMEOUT            ETIMEDOUT
#define CONN_HOSTUNREACH        EHOSTUNREACH
#define CONN_NETUNREACH         ENETUNREACH
#define CONN_NOTCONN            ENOTCONN
#define INFINITE                -1
#define CloseSocket             close

    using Socket = int32_t;

#elif defined(_WIN64_)

#if defined(errno)
#undef errno
#endif // defined(errno)
#define CONN_REFUSED            WSAECONNREFUSED 
#define CONN_TIMEOUT            WSAETIMEDOUT
#define CONN_HOSTUNREACH        WSAEHOSTUNREACH
#define CONN_NETUNREACH         WSAENETUNREACH
#define CONN_NOTCONN            WSAENOTCONN
#define errno                   WSAGetLastError()
#define CloseSocket             closesocket

    using Socket = SOCKET;

    enum EventType
    {
        INBOUND         = 0,
        OUTBOUND        = 1,
    };

#endif // defined(_POSIX_) 

    enum SocketType
    {
        ACCEPTER        = 0,
        CONNECTOR       = 1,
        MESSENGER       = 2,
    };

    struct IOContext
    {
#if defined(_WIN64_)
        OVERLAPPED      _ol;

        void Reset()
        {
            std::memset(&_ol, 0, sizeof(_ol));
        }
#elif defined(_POSIX_)
        void Reset()
        {}
#endif // _WIN64_
    };

    struct ReleaseContext : public IOContext
    {
        SocketSPtr      _sock { nullptr };
    };

    struct AcceptContext : public IOContext
    {
        Socket          _sock;

        char            _addr[(sizeof(sockaddr_storage) + 16) * 2] = { 0, };
#if defined(_WIN64_)
        DWORD           _len = 0; // addr len
#elif defined(_POSIX_) 
        socklen_t       _len = 0; // addr len
#endif // _WIN64_
        
        void Reset()
        {
            IOContext::Reset();
            _sock = INVALID_SOCKET;
            std::memset(_addr, 0, sizeof(_addr));
            _len = 0;
        }
    };

    struct AddrInfo
    {
        char            _addr[sizeof(sockaddr_storage)] = { 0, };
        size_t          _len = 0; // addr len

        void Reset()
        {
            std::memset(_addr, 0, sizeof(_addr));
            _len = 0;
        }
    };

    struct ConnectContext : public IOContext
    {
        std::vector<AddrInfo>   _addrs;
        std::size_t             _idx = 0;

        void Reset()
        {
            IOContext::Reset();
            _addrs.clear();
            _idx = 0;
        }
    };

    struct SendRecvContext : public IOContext
    {
        // send/recv buffer
        char            _buf[DEFAULT_TCP_SENDING_BUFFER_SIZE];

#if defined(_WIN64_)
        DWORD           _bytes = 0; // send/recv size
#elif defined(_POSIX_) 
        ssize_t         _bytes = 0; // send/recv size
#endif // _WIN64_

        // socket address for udp
        AddrInfo        _addrInfo;

        void Reset()
        {
            IOContext::Reset();
            std::memset(_buf, 0, sizeof(_buf));
            _bytes = 0;
            _addrInfo.Reset();
        }
    };

    struct IOResult
    {
        bool            _release = false;
        EventType       _eventType = EventType::INBOUND;
        SocketSPtr      _sock { nullptr };

        void Reset()
        {
            _release = false;
            _eventType = EventType::INBOUND;
            _sock.reset();
        }
    };
}
}

#endif // __ZS_NETWORK_INTERNAL_COMMON_H__
