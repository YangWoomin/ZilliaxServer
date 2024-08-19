
#include    "socket.h"

#include    "common/log.h"

#include    "helper.h"
#include    "manager.h"
#include    "socket_tcp_accepter.h"
#include    "socket_tcp_connector.h"

using namespace zs::common;
using namespace zs::network;

std::atomic<ConnectionID> ISocket::_connIDGen { 0 };

ISocket::ISocket(Manager& manager, SocketID sockID, SocketType type, IPVer ipVer, Protocol protocol, bool nonBlocking)
    : _manager(manager), _sockID(sockID), _type(type), _ipVer(ipVer), _protocol(protocol)
{
    _sock = Helper::CreateSocket(_ipVer, _protocol, nonBlocking);
}

ISocket::ISocket(Manager& manager, SocketID sockID, Socket sock, const std::string& name, const std::string& peer, SocketType type, IPVer ipVer, Protocol protocol)
    : _manager(manager), _sockID(sockID), _type(type), _sock(sock), _ipVer(ipVer), _protocol(protocol), _name(name), _peer(peer)
{

}

ISocket::~ISocket()
{
    if (INVALID_SOCKET != _sock)
    {
        CloseSocket(_sock);
    }
    _sock = INVALID_SOCKET;

    //if (nullptr != _onClosed && SocketType::ACCEPTER != _type)
    // if (nullptr != _onClosed)
    // {
    //     // invoke the callback function on accepter
    //     _onClosed(_conn);
    // }

    if (nullptr != _aCtx)
    {
        delete _aCtx;
        _aCtx = nullptr;
    }

    if (nullptr != _cCtx)
    {
        delete _cCtx;
        _cCtx = nullptr;
    }

    if (nullptr != _sCtx)
    {
        delete _sCtx;
        _sCtx = nullptr;
    }

    if (nullptr != _rCtx)
    {
        delete _rCtx;
        _rCtx = nullptr;
    }
}

bool ISocket::Bind(int32_t)
{
    ZS_LOG_FATAL(network, "binding on this socket is not implemented, sock id : %llu, name : %s", 
        _sockID, _name.c_str());

    return false;
}

bool ISocket::Listen(int32_t)
{
    ZS_LOG_FATAL(network, "listening on this socket is not implemented, sock id : %llu, name : %s", 
        _sockID, _name.c_str());

    return false;
}
bool ISocket::InitAccept()
{
    ZS_LOG_FATAL(network, "init accepting on this socket is not implemented, sock id : %llu, name : %s", 
        _sockID, _name.c_str());

    return false;
}

void ISocket::Close()
{
    if (true == _isBound.exchange(false))
    {
        ZS_LOG_WARN(network, "socket is being destroyed, sock id : %llu, socket name : %s, peer : %s",
            _sockID, GetName(), GetPeer());
    }

    if (INVALID_SOCKET != _sock)
    {
        CloseSocket(_sock);
    }
}

bool ISocket::InitConnect(const std::string&, int32_t)
{
    ZS_LOG_FATAL(network, "connecting on this socket is not implemented, sock id : %llu, name : %s", 
        _sockID, _name.c_str());

    return false;
}

bool ISocket::InitSend(std::string&&)
{
    ZS_LOG_FATAL(network, "init sending on this socket is not implemented, sock id : %llu, name : %s", 
        _sockID, _name.c_str());

    return false;
}

bool ISocket::InitSend(const std::string&)
{
    ZS_LOG_FATAL(network, "init sending on this socket is not implemented, sock id : %llu, name : %s", 
        _sockID, _name.c_str());

    return false;
}

bool ISocket::InitSend(const char*, std::size_t)
{
    ZS_LOG_FATAL(network, "init sending on this socket is not implemented, sock id : %llu, name : %s", 
        _sockID, _name.c_str());

    return false;
}

bool ISocket::ContinueSend()
{
    ZS_LOG_FATAL(network, "continuing sending on this socket is not implemented, sock id : %llu, name : %s", 
        _sockID, _name.c_str());

    return false;
}

bool ISocket::InitReceive()
{
    ZS_LOG_FATAL(network, "init receiving on this socket is not implemented, sock id : %llu, name : %s", 
        _sockID, _name.c_str());

    return false;
}

#if defined(_POSIX_) 
bool ISocket::Accept()
{
    ZS_LOG_FATAL(network, "accepting on this socket is not implemented, sock id : %llu, name : %s", 
        _sockID, _name.c_str());

    return false;
}

bool ISocket::Receive(bool&)
{
    ZS_LOG_FATAL(network, "receiving on this socket is not implemented, sock id : %llu, name : %s", 
        _sockID, _name.c_str());

    return false;
}
#endif // defined(_POSIX_) 

SocketSPtr ISocket::PostAccept()
{
    ZS_LOG_FATAL(network, "post accepting on this socket is not implemented, sock id : %llu, name : %s", 
        _sockID, _name.c_str());

    return nullptr;
}

bool ISocket::PostConnect(bool&)
{
    ZS_LOG_FATAL(network, "post connecting on this socket is not implemented, sock id : %llu, name : %s", 
        _sockID, _name.c_str());

    return false;
}

bool ISocket::PostSend()
{
    ZS_LOG_FATAL(network, "post sending on this socket is not implemented, sock id : %llu, name : %s", 
        _sockID, _name.c_str());

    return false;
}

bool ISocket::PostReceive()
{
    ZS_LOG_FATAL(network, "post receiving on this socket is not implemented, sock id : %llu, name : %s", 
        _sockID, _name.c_str());

    return false;
}

void ISocket::SetCallback(OnConnected onConnected, OnReceived onReceived, OnClosed onClosed)
{
    _onConnected = onConnected;
    _onReceived = onReceived;
    _onClosed = onClosed;
}

bool ISocket::bind(int32_t port)
{
    if (INVALID_SOCKET == _sock)
    {
        ZS_LOG_ERROR(network, "binding socket failed, sock id : %llu, socket name : %s", 
            _sockID, GetName());
        return false;
    }

    // bind the socket
    std::string name;
    sockaddr_storage addr;
    size_t len = 0;
    Helper::GenSockAddr(_ipVer, port, addr, len, name);
    if (SOCKET_ERROR == ::bind(_sock, (struct sockaddr*)&addr, (int32_t)len))
    {
        ZS_LOG_ERROR(network, "bind failed, socket id : %llu, err : %d", 
            _sockID, errno);
        return false;
    }
    
    _port = port;
    _name = name.c_str();
    _name += (":" + std::to_string(port));

    return true;
}

#if defined(_POSIX_) 
bool ISocket::modifyBindingOnDispatcher(EventType eventType)
{
    if (true == _isBound.load())
    {
        return _manager.Bind(_workerID, this, BindType::MODIFY, eventType);
    }
    else
    {
        ZS_LOG_ERROR(network, "this socket is being closed or not bound yet, socket id : %llu, socket name : %s, peer : %s", 
            _sockID, GetName(), GetPeer());
        return false;
    }
}
#endif // defined(_POSIX_)

/////////////////////////////////////////////////////////////////////////////
// SocketGenerator
SocketSPtr SocketGenerator::CreateSocket(Manager& manager, SocketID sockID, SocketType type, IPVer ipVer, Protocol protocol, bool nonBlocking)
{
    if (Protocol::TCP == protocol)
    {
        if (SocketType::ACCEPTER == type)
        {
            return SocketSPtr(new SocketTCPAceepter(manager, sockID, ipVer, nonBlocking));
        }
        else if (SocketType::CONNECTOR == type)
        {
            return SocketSPtr(new SocketTCPConnector(manager, sockID, ipVer, nonBlocking));
        }
        else if (SocketType::MESSENGER == type)
        {
            return SocketSPtr(new SocketTCPMessenger(manager, sockID, ipVer, nonBlocking));
        }
        else
        {
            ZS_LOG_ERROR(network, "invalid socket type, socket id : %llu, protocol : %d, type : %d", 
                sockID, protocol, type);
        }
    }
    else if (Protocol::UDP == protocol)
    {
        if (SocketType::ACCEPTER == type)
        {
            return nullptr;
        }
        else if (SocketType::CONNECTOR == type)
        {
            return nullptr;
        }
        else if (SocketType::MESSENGER == type)
        {
            return nullptr;
        }
        else
        {
            ZS_LOG_ERROR(network, "invalid socket type, socket id : %llu, protocol : %d, type : %d", 
                sockID, protocol, type);
        }
    }
    else
    {
        ZS_LOG_ERROR(network, "invalid protocol, socket id : %llu, protocol : %d", 
            sockID, protocol);
    }

    return nullptr;
}

SocketSPtr SocketGenerator::CreateSocket(Manager& manager, SocketID sockID, Socket sock, const std::string& name, const std::string& peer, SocketType type, IPVer ipVer, Protocol protocol)
{
    if (Protocol::TCP == protocol)
    {
        if (SocketType::ACCEPTER == type)
        {
            return SocketSPtr(new SocketTCPAceepter(manager, sockID, sock, name, peer, ipVer));
        }
        else if (SocketType::CONNECTOR == type)
        {
            return SocketSPtr(new SocketTCPConnector(manager, sockID, sock, name, peer, ipVer));
        }
        else if (SocketType::MESSENGER == type)
        {
            return SocketSPtr(new SocketTCPMessenger(manager, sockID, sock, name, peer, ipVer));
        }
        else
        {
            ZS_LOG_ERROR(network, "invalid socket type, socket id : %llu, protocol : %d, type : %d", 
                sockID, protocol, type);
        }
    }
    else if (Protocol::UDP == protocol)
    {
        if (SocketType::ACCEPTER == type)
        {
            return nullptr;
        }
        else if (SocketType::CONNECTOR == type)
        {
            return nullptr;
        }
        else if (SocketType::MESSENGER == type)
        {
            return nullptr;
        }
        else
        {
            ZS_LOG_ERROR(network, "invalid socket type, socket id : %llu, protocol : %d, type : %d", 
                sockID, protocol, type);
        }
    }
    else
    {
        ZS_LOG_ERROR(network, "invalid protocol, socket id : %llu, protocol : %d", 
            sockID, protocol);
    }

    return nullptr;
}
