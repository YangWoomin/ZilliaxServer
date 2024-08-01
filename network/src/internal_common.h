
#ifndef __ZS_NETWORK_INTERNAL_COMMON_H__
#define __ZS_NETWORK_INTERNAL_COMMON_H__

#if defined(_MSVC_)

#include    <Windows.h>
#include    <WinSock2.h>
#include    <ws2def.h>
#include    <ws2ipdef.h>
#include    <ws2tcpip.h>

#elif defined(__GNUC__) || defined(__clang__)

#include    <unistd.h>
#include    <errno.h>
#include    <sys/epoll.h>
#include    <sys/eventfd.h>
#include    <sys/socket.h>
#include    <arpa/inet.h>
#include    <netinet/in.h>

#endif // _MSVC_


#include    <memory>
#include    <vector>
#include    <string>
#include    <thread>

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
    static const std::size_t BUFFER_SIZE = 256;

    struct IOContext
    {
#if defined(_MSVC_)
        OVERLAPPED  _ol;
#else // _MSVC_
#endif // _MSVC_
        
        // send/recv buffer
        char        _buffer[BUFFER_SIZE] = { 0, };
    };

#if not defined(_MSVC_)
    enum BindType : int32_t
    {
        BIND            = EPOLL_CTL_ADD,
        UNBIND          = EPOLL_CTL_DEL,
        MODIFY          = EPOLL_CTL_MOD,
    };

    enum EventType
    {
        INBOUND         = EPOLLIN,      // triggered when some data received
        OUTBOUNT        = EPOLLOUT,     // triggered when socket affords to get sending data
    };

#define INVALID_SOCKET      -1
#define SOCKET_ERROR        -1
#define CloseSocket         close

    using Socket = int32_t;

#else // not _MSVC_

#define errno               WSAGetLastError()
#define CloseSocket         closesocket

    using Socket = SOCKET;

#endif // not _MSVC_

    enum SocketType
    {
        ACCEPTER        = 0,
        INPUT_OUTPUT    = 1,
    };

    struct SocketContext
    {
        char        _name[SOCKET_NAME_SIZE];
        Socket      _sock;
        SocketType  _sockType;
#if not defined(_MSVC_)
        BindType    _bindType;
        EventType   _eventType;
#endif // _MSVC_
    };
}
}

#endif // __ZS_NETWORK_INTERNAL_COMMON_H__
