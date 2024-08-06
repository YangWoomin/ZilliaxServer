
#include    "socket.h"

#include    "common/log.h"

#include    "helper.h"

using namespace zs::common;
using namespace zs::network;

/////////////////////////////////////////////////////////////////////////////
// ISocket
ISocket::ISocket(SocketID sockID, SocketType type, IPVer ipVer, Protocol protocol, bool nonBlocking)
    : _sockID(sockID), _type(type), _ipVer(ipVer), _protocol(protocol)
{
    _sock = Helper::CreateSocket(_ipVer, _protocol, nonBlocking);
}

void ISocket::Close()
{
    if (INVALID_SOCKET != _sock)
    {
        CloseSocket(_sock);
        _sock = INVALID_SOCKET;
    }

    ZS_LOG_WARN(network, "socket is being closed, sock id : %llu, name : %s",
        _sockID, GetName());
}

bool ISocket::bind(int32_t port)
{
    // bind the socket
    std::string name;
    sockaddr_storage addr;
    size_t len = 0;
    Helper::GenSockAddr(_ipVer, port, addr, len, name);
    if (SOCKET_ERROR == ::bind(_sock, (struct sockaddr*)&addr, (int32_t)len))
    {
        ZS_LOG_ERROR(network, "binding socket failed, socket id : %llu, err : %d", 
            _sockID, errno);
        return false;
    }
    
    _port = port;
    _name = name + ":" + std::to_string(port);

    return true;
}

/////////////////////////////////////////////////////////////////////////////
// SocketGenerator
SocketSPtr SocketGenerator::CreateSocket(SocketID sockID, SocketType type, IPVer ipVer, Protocol protocol, bool nonBlocking)
{
    if (Protocol::TCP == protocol)
    {
        if (SocketType::ACCEPTER == type)
        {
            return SocketSPtr(new SocketTCPListener(sockID, ipVer, nonBlocking));
        }
        else if (SocketType::CONNECTOR == type)
        {
            return SocketSPtr(new SocketTCPConnector(sockID, ipVer, nonBlocking));
        }
    }
    else if (Protocol::UDP == protocol)
    {

    }

    return nullptr;
}

/////////////////////////////////////////////////////////////////////////////
// SocketTCPListener
bool SocketTCPListener::Bind(int32_t port)
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

bool SocketTCPListener::Connect(const std::string& host, int32_t port, OnConnectedSPtr onConnected, OnReceivedSPtr onReceived)
{
    ZS_LOG_FATAL(network, "connecting listen socket is not supported, sock id : %llu, name : %s", 
        _sockID, _name.c_str());

    return false;
}

SocketTCPListener::SocketTCPListener(SocketID sockID, IPVer ipVer, bool nonBlocking)
    : ISocket(sockID, SocketType::ACCEPTER, ipVer, Protocol::TCP, nonBlocking)
{

}

SocketTCPListener::~SocketTCPListener()
{
    Close();
}

/////////////////////////////////////////////////////////////////////////////
// SocketTCPConnector
bool SocketTCPConnector::Bind(int32_t)
{
    if (INVALID_SOCKET == _sock)
    {
        ZS_LOG_ERROR(network, "invalid connect socket, name : %s", 
            GetName());
        return false;
    }

    if (false == this->bind(0))
    {
        ZS_LOG_ERROR(network, "binding connect socket failed, socket id : %llu", 
            _sockID, GetName());
        return false;
    }

    ZS_LOG_INFO(network, "binding connect socket succeeded, socket id : %llu, socket name : %s", 
        _sockID, GetName());

    return true;
}

bool SocketTCPConnector::Listen(int32_t backlog, OnConnectedSPtr onConnected, OnReceivedSPtr onReceived)
{
    ZS_LOG_FATAL(network, "listening connector socket is not supported, sock id : %llu, name : %s", 
        _sockID, _name.c_str());

    return false;
}

SocketTCPConnector::SocketTCPConnector(SocketID sockID, IPVer ipVer, bool nonBlocking)
    : ISocket(sockID, SocketType::CONNECTOR, ipVer, Protocol::TCP, nonBlocking)
{

}

SocketTCPConnector::~SocketTCPConnector()
{
    Close();
}

ConnectContext* SocketTCPConnector::prepare(const std::string& host, int32_t port)
{
    if (INVALID_SOCKET == _sock)
    {
        ZS_LOG_ERROR(network, "invalid connect socket created, socket name : %s", 
            GetName());
        return nullptr;
    }

    ConnectContext* cCtx = new ConnectContext();
    
    if (false == Helper::IsValidIP(host.c_str(), _ipVer))
    {
        // if host is passed as domain name
        // convert it to ip(s) and try to connect by the ip(s)
        struct addrinfo hints, *res;

        Helper::GenAddrInfo(_protocol, hints);

        // this could be blocked
        if (0 != getaddrinfo(host.c_str(), NULL, &hints, &res))
        {
            ZS_LOG_ERROR(network, "getaddrinfo for connect failed, socket name : %s, err : %d",
                GetName(), errno);
            delete cCtx;
            return nullptr;
        }

        for (auto p = res; NULL != p; p = p->ai_next)
        {
            AddrInfo addrInfo;
            Helper::ConvertAddrInfoToSockAdddr(p, port, (sockaddr_storage*)addrInfo._addr, addrInfo._len);
            cCtx->_addrs.push_back(addrInfo);
        }
    }
    else
    {
        AddrInfo addrInfo;
        Helper::GenSockAddr(_ipVer, host.c_str(), port, (sockaddr_storage*)addrInfo._addr, addrInfo._len);
        cCtx->_addrs.push_back(addrInfo);
    }

    if (0 >= cCtx->_addrs.size())
    {
        ZS_LOG_ERROR(network, "getting ip from hostname failed, name : %s", 
            GetName());
        delete cCtx;
        return nullptr;
    }

    return cCtx;
}
