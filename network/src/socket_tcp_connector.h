
#ifndef __ZS_NETWORK_SOCKET_TCP_CONNECTOR_H__
#define __ZS_NETWORK_SOCKET_TCP_CONNECTOR_H__

#include    "socket_tcp_messenger.h"

namespace zs
{
namespace network
{
    class Manager;
    
    class SocketTCPConnector : public SocketTCPMessenger
    {
    public:
        // bind a socket by calling "bind"
        // bind the socket to dispatcher for connected event
        virtual bool Bind(int32_t) override;

        // make the socket connect to remote host by calling "connect"
        // if in windows initiate async connect
        // if in linux initiate non-blocking connect
        virtual bool Connect(const std::string& host, int32_t port) override;

        virtual bool PreConnect(std::size_t idx) override;

        virtual bool PostConnect(bool& retry) override;

        virtual ~SocketTCPConnector();

    private:
        SocketTCPConnector(Manager& manager, SocketID sockID, IPVer ipVer, bool nonBlocking);
        SocketTCPConnector(Manager& manager, SocketID sockID, Socket sock, const std::string& name, const std::string& peer, IPVer ipVer);

        SocketTCPConnector(const SocketTCPConnector&) = delete;
        SocketTCPConnector(const SocketTCPConnector&&) = delete;
        SocketTCPConnector& operator=(const SocketTCPConnector&) = delete;

        bool prepare(const std::string& host, int32_t port);
        bool postConnect();

        friend class SocketGenerator;
    };
}
}

#endif // __ZS_NETWORK_SOCKET_TCP_CONNECTOR_H__

