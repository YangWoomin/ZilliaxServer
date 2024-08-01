
#ifndef __ZS_NETWORK_NETWORK_H__
#define __ZS_NETWORK_NETWORK_H__

#include    "common/types.h"
#include    "common/log.h"

#include    "network/common.h"

#include    <functional>

namespace zs
{
namespace network
{
    using namespace zs::common;
    
    class __ZS_NETWORK_API Network
    {
    public:
        static bool Initialize(Logger::Messenger msgr);
        static void Finalize();

        static bool Start(std::size_t workerCount = 0);
        static void Stop();

        static bool Bind(IPVer ipVer, Protocol protocol, int32_t port, SocketID& sockID);
        static bool Connect(IPVer ipVer, Protocol protocol, int32_t port);
        static void Close(SocketID sID);

    private:
        static const std::size_t        MAX_WORKER_COUNT = 1024;
        static const int32_t            AVAILABLE_MINIMUM_PORT = 1024;
        static const int32_t            AVAILABLE_MAXIMUM_PORT = 65535;

        Network() = delete;
        ~Network() = delete;
        Network(const Network&) = delete;
        Network& operator=(const Network&) = delete;

    friend class Manager;
    };
}
}

#endif // __ZS_NETWORK_NETWORK_H__
