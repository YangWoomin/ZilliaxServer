
#include    "socket_tcp_connector.h"

#include    "common/log.h"

#include    "helper.h"
#include    "manager.h"

#include    "network/connection.h"

using namespace zs::common;
using namespace zs::network;

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

bool SocketTCPConnector::InitConnect(const std::string& host, int32_t port)
{
    if (INVALID_SOCKET == _sock)
    {
        ZS_LOG_ERROR(network, "invalid connect socket in init connect, sock id : %llu, socket name : %s", 
            _sockID, GetName());
        return false;
    }

    if (false == prepare(host, port))
    {
        ZS_LOG_ERROR(network, "preparing failed in init connect, sock id : %llu, socket name : %s",
            _sockID, GetName());
        return false;
    }

    if (false == initConnect(_cCtx->_idx))
    {
        ZS_LOG_ERROR(network, "internal init connecting in init connect failed, sock id : %llu, socket name : %s",
            _sockID, GetName());
        return false;
    }

    ZS_LOG_INFO(network, "init connect succeeded, socket id : %llu, socket name : %s, host : %s, port : %d", 
        _sockID, GetName(), host.c_str(), port);

    return true;
}

bool SocketTCPConnector::PostConnect(bool& retry)
{
    if (INVALID_SOCKET == _sock)
    {
        ZS_LOG_ERROR(network, "invalid connect socket created, socket name : %s", 
            GetName());
        return false;
    }

    int err = 0;
    socklen_t len = sizeof(err);
    if (SOCKET_ERROR == getsockopt(_sock, SOL_SOCKET, SO_ERROR, (char*)&err, &len))
    {
        ZS_LOG_ERROR(network, "getsockopt failed in post connect, sock id : %llu, socket name : %s, err : %d",
            _sockID, GetName(), errno);
        return false;
    }

    // check whether the connection to remote is established well
    // if not, retry to connect with the other ip (in context)
    // if all of tries failed then invoke onConnected with null pointer of connection
    if (NO_SOCKET_ERROR != err)
    {
        if (CONN_REFUSED == err || CONN_TIMEOUT == err || 
            CONN_HOSTUNREACH == err || CONN_NETUNREACH == err)
        {
            if (_cCtx->_addrs.size() <= _cCtx->_idx)
            {
                ZS_LOG_ERROR(network, "no triable ip in post connect, sock id : %llu, socket name : %s", 
                    _sockID, GetName());
                return false;
            }

            if (false == initConnect(++_cCtx->_idx))
            {
                ZS_LOG_ERROR(network, "retrying to connect failed in post connect, sock id : %llu, socket name : %s", 
                    _sockID, GetName());
                return false;
            }

            ZS_LOG_WARN(network, "retrying to connect in post connect, sock id : %llu, socket name : %s, err : %d",
                _sockID, GetName(), err);

            retry = true;
            return true;
        }

        ZS_LOG_ERROR(network, "socket error occurred in post connect, sock id : %llu, socket name : %s", 
            _sockID, GetName());
        
        if (nullptr != _onConnected)
        {
            (*_onConnected)(ConnectionSPtr(nullptr));
        }

        return false;
    }

    if (false == postConnect())
    {
        ZS_LOG_ERROR(network, "internal post connect failed, sock id : %llu, socket name : %s",
            _sockID, GetName());
        return false;
    }

    int32_t remotePort = 0;
    Helper::GetSockRemoteAddr(_sock, _ipVer, _peer, remotePort);
    if (true == _peer.empty())
    {
        ZS_LOG_ERROR(network, "GetSockRemoteAddr failed in post connect, sock id : %llu, socket name : %s",
            _sockID, GetName());
        return false;
    }

    _peer = _peer.c_str();
    _peer += (":" + std::to_string(remotePort));

    // create a connection for the connected socket
    ConnectionSPtr conn = ConnectionSPtr(new Connection(++_connIDGen, GetSPtr()));
    _conn = conn;

    // invoke onConnected with the connection as a parameter
    if (nullptr != _onConnected)
    {
        (*_onConnected)(conn);
    }

    // change the socket type from connector to messenger
    ChangeType(SocketType::MESSENGER);

    ZS_LOG_INFO(network, "post connect succeeded, sock id : %llu, socket name : %s, peer : %s",
        _sockID, GetName(), GetPeer());

    return true;
}

bool SocketTCPConnector::prepare(const std::string& host, int32_t port)
{
    if (INVALID_SOCKET == _sock)
    {
        ZS_LOG_ERROR(network, "invalid connect socket created, socket name : %s", 
            GetName());
        return false;
    }

    if (nullptr == _cCtx)
    {
        _cCtx = new ConnectContext();
    }
    _cCtx->Reset();
    
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
            return false;
        }

        for (auto p = res; NULL != p; p = p->ai_next)
        {
            AddrInfo addrInfo;
            Helper::ConvertAddrInfoToSockAdddr(p, port, (sockaddr_storage*)addrInfo._addr, addrInfo._len);
            _cCtx->_addrs.push_back(addrInfo);
        }
    }
    else
    {
        AddrInfo addrInfo;
        Helper::GenSockAddr(_ipVer, host.c_str(), port, (sockaddr_storage*)addrInfo._addr, addrInfo._len);
        _cCtx->_addrs.push_back(addrInfo);
    }

    if (0 >= _cCtx->_addrs.size())
    {
        ZS_LOG_ERROR(network, "getting ip from hostname failed, name : %s", 
            GetName());
        return false;
    }

    return true;
}

SocketTCPConnector::SocketTCPConnector(Manager& manager, SocketID sockID, IPVer ipVer, bool nonBlocking)
    : SocketTCPMessenger(manager, sockID, ipVer, nonBlocking)
{
    _type = SocketType::CONNECTOR;
}

SocketTCPConnector::SocketTCPConnector(Manager& manager, SocketID sockID, Socket sock, const std::string& name, const std::string& peer, IPVer ipVer)
    : SocketTCPMessenger(manager, sockID, sock, name, peer, ipVer)
{
    _type = SocketType::CONNECTOR;
}

SocketTCPConnector::~SocketTCPConnector()
{
    Close();
}