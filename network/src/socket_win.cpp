
#include    "socket.h"

#include    "common/log.h"

#include    "helper.h"

using namespace zs::common;
using namespace zs::network;

#if defined(_MSVC_)

/////////////////////////////////////////////////////////////////////////////
// SocketTCPListener
bool SocketTCPListener::Listen(int32_t backlog, OnConnectedSPtr onConnected, OnReceivedSPtr onReceived)
{
    if (INVALID_SOCKET == _sock)
    {
        ZS_LOG_ERROR(network, "invalid listen socket, sock id : %llu, socket name : %s", 
            _sockID, GetName());
        return false;
    }

    if (nullptr == Helper::_lpfnAcceptEx)
    {
        ZS_LOG_ERROR(network, "invalid acceptex, sock id : %llu, socket name : %s",
            _sockID, GetName());
        return false;
    }

    if (SOCKET_ERROR == listen(_sock, backlog))
    {
        ZS_LOG_ERROR(network, "listen failed, sock id : %llu, socket name : %s, err : %d", 
            _sockID, GetName(), errno);
        return false;
    }

    // AcceptEx
    Socket sock = Helper::CreateSocket(_ipVer, _protocol);
    if (INVALID_SOCKET == sock)
    {
        ZS_LOG_ERROR(network, "creating socket for listen failed, sock id : %llu, socket name : %s",
            _sockID, GetName());
        return false;
    }

    AcceptContext* aCtx = new AcceptContext();
    std::memset(aCtx, 0, sizeof(AcceptContext));
    aCtx->_sock = sock;
    if (FALSE == Helper::_lpfnAcceptEx(_sock, sock, aCtx->_addr, 0, sizeof(sockaddr_storage) + 16, sizeof(sockaddr_storage) + 16, &aCtx->_len, &aCtx->_ol))
    {
        int err = errno;
        if (WSA_IO_PENDING != err)
        {
            ZS_LOG_ERROR(network, "acceptex failed, sock id : %llu, socket name : %s, err : %d",
                _sockID, GetName(), err);
            CloseSocket(sock);
            delete aCtx;
            return false;
        }
    }

    _onConnected = onConnected;
    _onReceived = onReceived;

    ZS_LOG_INFO(network, "listen succeeded, socket id : %llu, socket name : %s", 
        _sockID, GetName());

    return true;
}

/////////////////////////////////////////////////////////////////////////////
// SocketTCPConnector
bool SocketTCPConnector::Connect(const std::string& host, int32_t port, OnConnectedSPtr onConnected, OnReceivedSPtr onReceived)
{
    if (INVALID_SOCKET == _sock)
    {
        ZS_LOG_ERROR(network, "invalid connect socket, sock id : %llu, socket name : %s", 
            _sockID, GetName());
        return false;
    }

    if (nullptr == Helper::_lpfnConnectEx)
    {
        ZS_LOG_ERROR(network, "invalid connectex, sock id : %llu, socket name : %s",
            _sockID, GetName());
        return false;
    }

    ConnectContext* cCtx = prepare(host, port);
    if (nullptr == cCtx)
    {
        ZS_LOG_ERROR(network, "preparing for connect failed, sock id : %llu, socket name : %s",
            _sockID, GetName());
        return false;
    }

    // ConnectEx
    if (FALSE == Helper::_lpfnConnectEx(_sock, (const sockaddr*)cCtx->_addrs[cCtx->_idx]._addr, (int32_t)cCtx->_addrs[cCtx->_idx]._len, NULL, 0, NULL, &cCtx->_ol))
    {
        int err = errno;
        if (WSA_IO_PENDING != err)
        {
            ZS_LOG_ERROR(network, "connectex failed, sock id : %llu, socket name : %s, err : %d",
                _sockID, GetName(), err);
            delete cCtx;
            return false;
        }
    }

    _onConnected = onConnected;
    _onReceived = onReceived;

    ZS_LOG_INFO(network, "connecting succeeded, socket id : %llu, socket name : %s, host : %s, port : %d", 
        _sockID, GetName(), host.c_str(), port);

    return true;
}

#endif // _MSVC_

