
#include    "socket.h"

#include    "common/log.h"

using namespace zs::common;
using namespace zs::network;

#if defined(__GNUC__) || defined(__clang__)

/////////////////////////////////////////////////////////////////////////////
// SocketTCPListener
bool SocketTCPListener::Listen(int32_t backlog, OnConnectedSPtr onConnected, OnReceivedSPtr onReceived)
{
    if (INVALID_SOCKET == _sock)
    {
        ZS_LOG_ERROR(network, "not bound listen socket, sock id : %llu, socket name : %s", 
            _sockID, GetName());
        return false;
    }

    if (SOCKET_ERROR == listen(_sock, backlog))
    {
        ZS_LOG_ERROR(network, "listen failed, sock id : %llu, socket name : %s, err : %d", 
            _sockID, GetName(), errno);
        return false;
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

    ConnectContext* cCtx = prepare(host, port);
    if (nullptr == cCtx)
    {
        ZS_LOG_ERROR(network, "preparing for connect failed, sock id : %llu, socket name : %s",
            _sockID, GetName());
        return false;
    }

    if (INVALID_RESULT == connect(_sock, (const sockaddr*)cCtx->_addrs[cCtx->_idx]._addr, cCtx->_addrs[cCtx->_idx]._len))
    {
        ZS_LOG_ERROR(network, "connecting failed, sock id : %llu, socket name : %s, err : %d",
            _sockID, GetName(), errno);
        return false;
    }

    _onConnected = onConnected;
    _onReceived = onReceived;

    _ctx = cCtx;

    ZS_LOG_INFO(network, "connecting succeeded, socket id : %llu, socket name : %s, host : %s, port : %d", 
        _sockID, GetName(), host.c_str(), port);

    return true;
}

#endif // defined(__GNUC__) || defined(__clang__)
