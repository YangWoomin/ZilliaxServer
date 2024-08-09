
#include    "socket_tcp_accepter.h"

#include    "common/log.h"

#include    "helper.h"
#include    "manager.h"

using namespace zs::common;
using namespace zs::network;

bool SocketTCPAceepter::Bind(int32_t port)
{
    if (INVALID_SOCKET == _sock)
    {
        ZS_LOG_ERROR(network, "invalid listen socket, name : %s", 
            GetName());
        return false;
    }

    // set socket reuse option
    int reuse = 1;
    if (SOCKET_ERROR == setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)))
    {
        ZS_LOG_ERROR(network, "setsockopt reuse for listen socket failed, socket id : %llu, err : %d", 
            _sockID, errno);
        return false;
    }

    if (false == this->bind(port))
    {
        ZS_LOG_ERROR(network, "binding listen socket failed, socket id : %llu", 
            _sockID);
        return false;
    }

    ZS_LOG_INFO(network, "binding listen socket succeeded, socket id : %llu, name : %s", 
        _sockID, GetName());

    return true;
}

bool SocketTCPAceepter::Listen(int32_t backlog)
{
    if (INVALID_SOCKET == _sock)
    {
        ZS_LOG_ERROR(network, "listening on socket failed, sock id : %llu, socket name : %s", 
            _sockID, GetName());
        return false;
    }

    if (SOCKET_ERROR == listen(_sock, backlog))
    {
        ZS_LOG_ERROR(network, "listen failed, sock id : %llu, socket name : %s, err : %d", 
            _sockID, GetName(), errno);
        return false;
    }

    if (false == PreAccept())
    {
        ZS_LOG_ERROR(network, "pre accepting failed, sock id : %llu, socket name : %s, err : %d", 
            _sockID, GetName(), errno);
        return false;
    }

    ZS_LOG_INFO(network, "listening succeeded, socket id : %llu, socket name : %s", 
        _sockID, GetName());

    return true;
}

SocketTCPAceepter::SocketTCPAceepter(Manager& manager, SocketID sockID, IPVer ipVer, bool nonBlocking)
    : ISocket(manager, sockID, SocketType::ACCEPTER, ipVer, Protocol::TCP, nonBlocking)
{

}

SocketTCPAceepter::SocketTCPAceepter(Manager& manager, SocketID sockID, Socket sock, const std::string& name, const std::string& peer, IPVer ipVer)
    : ISocket(manager, sockID, sock, name, peer, SocketType::ACCEPTER, ipVer, Protocol::TCP)
{

}

SocketTCPAceepter::~SocketTCPAceepter()
{
    Close();
}
