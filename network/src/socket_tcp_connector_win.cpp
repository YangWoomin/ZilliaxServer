
#include    "socket_tcp_connector.h"

#include    "common/log.h"

#include    "helper.h"
#include    "manager.h"

using namespace zs::common;
using namespace zs::network;

#if defined(_WIN64_)

bool SocketTCPConnector::initConnect(std::size_t idx)
{
    if (INVALID_SOCKET == _sock)
    {
        ZS_LOG_ERROR(network, "invalid connect socket in internal init connect, sock id : %llu, socket name : %s", 
            _sockID, GetName());
        return false;
    }

    if (nullptr == Helper::_lpfnConnectEx)
    {
        ZS_LOG_ERROR(network, "invalid connectex, sock id : %llu, socket name : %s",
            _sockID, GetName());
        return false;
    }

    // ConnectEx
    if (FALSE == Helper::_lpfnConnectEx(_sock, (const sockaddr*)_cCtx->_addrs[idx]._addr, (int32_t)_cCtx->_addrs[idx]._len, NULL, 0, NULL, &_cCtx->_ol))
    {
        int err = errno;
        if (WSA_IO_PENDING != err)
        {
            ZS_LOG_ERROR(network, "connectex failed, sock id : %llu, socket name : %s, err : %d",
                _sockID, GetName(), err);
            return false;
        }
    }

    std::string remoteHost;
    int32_t remotePort = 0;
    Helper::GetSockAddr((sockaddr*)_cCtx->_addrs[idx]._addr, remoteHost, remotePort);

    ZS_LOG_INFO(network, "internal init connecting succeeded, socket id : %llu, socket name : %s, remote host : %s, remote port : %d", 
        _sockID, GetName(), remoteHost.c_str(), remotePort);

    return true;
}

bool SocketTCPConnector::postConnect()
{
    if (INVALID_SOCKET == _sock)
    {
        ZS_LOG_ERROR(network, "invalid connect socket in post connect, sock id : %llu, socket name : %s", 
            _sockID, GetName());
        return false;
    }

    if (SOCKET_ERROR == setsockopt(_sock, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0))
    {
        ZS_LOG_ERROR(network, "setsockopt failed in post connect, sock id : %llu, socket name : %s", 
            _sockID, GetName());
        return false;
    }

    return true;
}

#endif // _WIN64_
