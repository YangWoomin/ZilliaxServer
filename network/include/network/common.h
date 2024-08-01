
#ifndef __ZS_NETWORK_COMMON_H__
#define __ZS_NETWORK_COMMON_H__

#if defined(_MSVC_)

#ifdef ZS_NETWORK_EXPORTS
#define __ZS_NETWORK_API __declspec(dllexport)
#else // ZS_NETWORK_EXPORTS
#define __ZS_NETWORK_API __declspec(dllimport)
#endif

#elif defined(__GNUC__) || defined(__clang__)

#ifdef ZS_NETWORK_EXPORTS
#define __ZS_NETWORK_API __attribute__((visibility ("default")))
#else // ZS_NETWORK_EXPORTS
#define __ZS_NETWORK_API 
#endif

#endif // _MSVC_


#include    <memory>

namespace zs
{
namespace network
{
    class Connection;
    using ConnectionSPtr = std::shared_ptr<Connection>;

    using SocketID = uint64_t;

    enum class __ZS_NETWORK_API IPVer
    {
        IP_V4       = 1,
        IP_V6       = 2, // not supported
    };

    enum class __ZS_NETWORK_API Protocol
    {
        TCP         = 1,
        UDP         = 2,
    };
}
}

#endif // __ZS_NETWORK_COMMON_H__
