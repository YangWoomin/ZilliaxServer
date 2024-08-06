
#ifndef __ZS_NETWORK_INTERNAL_COMMON_H__
#define __ZS_NETWORK_INTERNAL_COMMON_H__

#if defined(_MSVC_)

#include    <WinSock2.h>
#include    <mswsock.h>
#include    <Windows.h>
#include    <ws2def.h>
#include    <ws2ipdef.h>
#include    <ws2tcpip.h>

#elif defined(__GNUC__) || defined(__clang__)

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

#endif // _MSVC_


#include    "network/common.h"

#include    <memory>
#include    <vector>
#include    <string>
#include    <thread>
#include    <atomic>

namespace zs
{
namespace network
{
    class Worker;
    using WorkerSPtr = std::shared_ptr<Worker>;
    using Workers = std::vector<WorkerSPtr>;

    class Dispatcher;
    using DispatcherSPtr = std::shared_ptr<Dispatcher>;

    static const std::size_t SOCKET_NAME_SIZE = 128;
    static const std::size_t BUFFER_SIZE = 1024;

#if defined(__GNUC__) || defined(__clang__)
    enum BindType : int32_t
    {
        BIND            = EPOLL_CTL_ADD,
        UNBIND          = EPOLL_CTL_DEL,
        MODIFY          = EPOLL_CTL_MOD,
    };

    enum EventType
    {
        INBOUND         = EPOLLIN,      // triggered when some data is received or a remote client is accepted
        OUTBOUND        = EPOLLOUT,     // triggered when socket affords to get sending data or a remote client is connected
    };

#define INVALID_FD_VALUE        -1
#define INVALID_SOCKET          -1
#define INVALID_RESULT          -1
#define SOCKET_ERROR            -1
#define INFINITE                -1
#define CloseSocket             close

    using Socket = int32_t;

#elif defined(_MSVC_)

#if defined(errno)
#undef errno
#endif // defined(errno)
#define errno                   WSAGetLastError()
#define CloseSocket             closesocket

    using Socket = SOCKET;

#endif // defined(__GNUC__) || defined(__clang__)

    class ISocket;
    using SocketSPtr = std::shared_ptr<ISocket>;

    enum SocketType
    {
        ACCEPTER        = 0,
        CONNECTOR       = 1,
        MESSENGER       = 2,
    };

    struct IOContext
    {
#if defined(_MSVC_)
        OVERLAPPED      _ol;        
#endif // _MSVC_
    };

    struct AcceptContext : public IOContext
    {
        Socket          _sock;
        char            _addr[(sizeof(sockaddr_storage) + 16) * 2] = { 0, };
#if defined(_MSVC_)
        DWORD           _len = 0; // addr len
#elif defined(__GNUC__) || defined(__clang__)
        socklen_t       _len = 0; // addr len
#endif // _MSVC_
    };

    struct AddrInfo
    {
        char            _addr[sizeof(sockaddr_storage)] = { 0, };
        size_t          _len = 0; // addr len
    };

    struct ConnectContext : public IOContext
    {
        std::vector<AddrInfo>   _addrs;
        std::size_t             _idx = 0;
    };

    struct SendRecvContext : public IOContext
    {
        // whether this context is for recv or send
        bool            _isRecv = true;

        // send/recv buffer
        char            _buf[BUFFER_SIZE] = { 0, };

        // socket address for udp
        AddrInfo        _addrInfo;
    };

    struct Result
    {
        bool            _stop = false;
        bool            _release = false;
#if defined(_MSVC_)
        DWORD           _bytes = 0; // io result size
#elif defined(__GNUC__) || defined(__clang__)
        int32_t         _bytes = 0; // io result size
#endif // _MSVC_
    };

    struct ResultItem final
    {
        Result          _res;
        ISocket*        _sock = nullptr;
        IOContext*      _iCtx = nullptr;
    };
}
}

#endif // __ZS_NETWORK_INTERNAL_COMMON_H__
