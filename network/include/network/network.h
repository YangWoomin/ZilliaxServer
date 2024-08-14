
#ifndef __ZS_NETWORK_NETWORK_H__
#define __ZS_NETWORK_NETWORK_H__

#include    "common/types.h"
#include    "common/log.h"

#include    "network/common.h"

namespace zs
{
namespace network
{
    using namespace zs::common;
    
    class __ZS_NETWORK_API Network final
    {
    public:
        // these functions should be called by only one(main) thread when program start or end time
        static bool Initialize(Logger::Messenger msgr);
        static void Finalize();
        static bool Start(std::size_t dispatcherWorkerCount = 0);
        static void Stop();

        // these functions should be called by one thread for one socket
        // DO NOT call these functions simultaneously for one socket
        // these functions are available for simultaneous call for different sockets
        static bool Bind(IPVer ipVer, Protocol protocol, int32_t port, SocketID& sockID);
        static bool Listen(SocketID sockID, int32_t backlog, OnConnectedSPtr onConnected, OnReceivedSPtr onReceived, OnClosedSPtr onClosed);
        static bool Close(SocketID sockID);
        
        static bool ConnectTCP(IPVer ipVer, std::string host, int32_t port, OnConnectedSPtr onConnected, OnReceivedSPtr onReceived, OnClosedSPtr onClosed);
        static ConnectionSPtr ConnectUDP(IPVer ipVer, std::string host, int32_t port, OnReceivedSPtr onReceived);

        static const std::size_t        MAX_WORKER_COUNT = 128;
        static const int32_t            AVAILABLE_MINIMUM_PORT = 1024;
        static const int32_t            AVAILABLE_MAXIMUM_PORT = 65535;
        static const int32_t            MAX_BACKLOG_SIZE = 1024;
    
    private:

        Network() = delete;
        ~Network() = delete;
        Network(const Network&) = delete;
        Network& operator=(const Network&) = delete;

    friend class Manager;
    };
}
}

#endif // __ZS_NETWORK_NETWORK_H__
