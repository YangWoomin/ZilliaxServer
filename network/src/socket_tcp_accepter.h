
#ifndef __ZS_NETWORK_SOCKET_TCP_ACCEPTER_H__
#define __ZS_NETWORK_SOCKET_TCP_ACCEPTER_H__

#include    "socket.h"

namespace zs
{
namespace network
{
    class Manager;
    
    class SocketTCPAceepter : public ISocket
    {
    public:
        // bind a socket by calling "bind"
        // bind the socket to dispatcher for accepted event
        virtual bool Bind(int32_t port) override;

        // make the socket listening by calling "listen"
        // if in windows initiate async accept for the next accepted socket
        virtual bool Listen(int32_t backlog) override;

        virtual bool InitAccept() override;

#if defined(_POSIX_) 
        virtual bool Accept() override;
#endif // defined(_POSIX_) 

        virtual SocketSPtr PostAccept() override;

    private:
        bool postAccept(std::string& name, std::string& peer);

        SocketTCPAceepter(Manager& manager, SocketID sockID, IPVer ipVer, bool nonBlocking);
        SocketTCPAceepter(Manager& manager, SocketID sockID, Socket sock, const std::string& name, const std::string& peer, IPVer ipVer);
        
        SocketTCPAceepter(const SocketTCPAceepter&) = delete;
        SocketTCPAceepter(const SocketTCPAceepter&&) = delete;
        SocketTCPAceepter& operator=(const SocketTCPAceepter&) = delete;

    public:
        virtual ~SocketTCPAceepter();

        friend class SocketGenerator;
    };
}
}

#endif // __ZS_NETWORK_SOCKET_TCP_ACCEPTER_H__
