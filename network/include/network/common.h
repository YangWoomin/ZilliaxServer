
#ifndef __ZS_NETWORK_COMMON_H__
#define __ZS_NETWORK_COMMON_H__

#if defined(_WIN64_)

#ifdef ZS_NETWORK_EXPORTS
#define __ZS_NETWORK_API __declspec(dllexport)
#else // ZS_NETWORK_EXPORTS
#define __ZS_NETWORK_API __declspec(dllimport)
#endif

#elif defined(_POSIX_) 

#ifdef ZS_NETWORK_EXPORTS
#define __ZS_NETWORK_API __attribute__((visibility ("default")))
#else // ZS_NETWORK_EXPORTS
#define __ZS_NETWORK_API 
#endif

#endif // _WIN64_


#include    <memory>
#include    <functional>

namespace zs
{
namespace network
{
    class Connection;
    using ConnectionSPtr = std::shared_ptr<Connection>;
    using ConnectionID = uint64_t;

    using OnConnected = std::function<void(ConnectionSPtr)>;
    using OnConnectedSPtr = std::shared_ptr<OnConnected>;

    using OnReceived = std::function<void(ConnectionSPtr, const char*, std::size_t)>;
    using OnReceivedSPtr = std::shared_ptr<OnReceived>;

    using OnClosed = std::function<void(ConnectionSPtr)>;
    using OnClosedSPtr = std::shared_ptr<OnClosed>;

    class ISocket;
    using SocketSPtr = std::shared_ptr<ISocket>;
    using SocketWPtr = std::weak_ptr<ISocket>;
    using SocketID = uint64_t;

    enum class __ZS_NETWORK_API IPVer
    {
        IP_INVALID  = 0,
        IP_V4       = 1,
        IP_V6       = 2,
    };

    enum class __ZS_NETWORK_API Protocol
    {
        INVALID     = 0,
        TCP         = 1,
        UDP         = 2,
    };

    static const uint32_t           DEFAULT_TCP_RECVING_BUFFER_SIZE = 4000; // 4KiB (approximately)
    static const uint32_t           DEFAULT_TCP_SENDING_BUFFER_SIZE = 1024;
    static const uint32_t           DEFAULT_TCP_SENDING_BUFFER_COUNT = 8; // maximum 4KiB * DEFAULT_TCP_SENDING_BUFFER_COUNT
}
}

#endif // __ZS_NETWORK_COMMON_H__
