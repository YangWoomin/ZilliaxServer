
#ifndef __ZS_NETWORK_COMMON_H__
#define __ZS_NETWORK_COMMON_H__

#if defined(_WIN64_)

#ifdef ZS_NETWORK_EXPORTS
#define __ZS_NETWORK_API __declspec(dllexport)
#else // ZS_NETWORK_EXPORTS
#define __ZS_NETWORK_API __declspec(dllimport)
#endif

#elif defined(_LINUX_) 

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
    using ConnectionWPtr = std::weak_ptr<Connection>;
    using ConnectionID = uint64_t;

    using OnConnected = std::function<void(ConnectionWPtr)>;
    using OnConnectedSPtr = std::shared_ptr<OnConnected>;

    using OnReceived = std::function<void(ConnectionWPtr, const char*, std::size_t)>;
    using OnReceivedSPtr = std::shared_ptr<OnReceived>;

    using OnClosed = std::function<void(ConnectionWPtr)>;
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

    static const std::size_t BUFFER_SIZE = 1024 * 1024; // maxinum message size
}
}

#endif // __ZS_NETWORK_COMMON_H__
