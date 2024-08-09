
#ifndef __ZS_NETWORK_SOCKET_TCP_MESSENGER_H__
#define __ZS_NETWORK_SOCKET_TCP_MESSENGER_H__

#include    "socket.h"

namespace zs
{
namespace network
{
    class Manager;

    class SocketTCPMessenger : public ISocket
    {
    public:
        virtual bool Send() override;

        virtual bool PreRecv(bool& isReceived) override;

        virtual ~SocketTCPMessenger();

    private:
        SocketTCPMessenger(Manager& manager, SocketID sockID, IPVer ipVer, bool nonBlocking);
        SocketTCPMessenger(Manager& manager, SocketID sockID, Socket sock, const std::string& name, const std::string& peer, IPVer ipVer);
        

        SocketTCPMessenger(const SocketTCPMessenger&) = delete;
        SocketTCPMessenger(const SocketTCPMessenger&&) = delete;
        SocketTCPMessenger& operator=(const SocketTCPMessenger&) = delete;

        friend class SocketGenerator;
        friend class SocketTCPConnector;
    };
}
}

#endif // __ZS_NETWORK_SOCKET_TCP_MESSENGER_H__

