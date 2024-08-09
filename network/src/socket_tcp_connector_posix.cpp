
#include    "socket_tcp_connector.h"

#include    "common/log.h"

#include    "helper.h"
#include    "manager.h"

using namespace zs::common;
using namespace zs::network;

#if defined(__GNUC__) || defined(__clang__)

bool SocketTCPConnector::Connect(const std::string& host, int32_t port)
{
    if (INVALID_SOCKET == _sock)
    {
        ZS_LOG_ERROR(network, "invalid connect socket, sock id : %llu, socket name : %s", 
            _sockID, GetName());
        return false;
    }

    if (false == prepare(host, port))
    {
        ZS_LOG_ERROR(network, "preparing for connect failed, sock id : %llu, socket name : %s",
            _sockID, GetName());
        return false;
    }

    if (false == PreConnect(_cCtx->_idx))
    {
        ZS_LOG_ERROR(network, "pre connecting failed, sock id : %llu, socket name : %s",
            _sockID, GetName());
        return false;
    }

    ZS_LOG_INFO(network, "connecting succeeded, socket id : %llu, socket name : %s, host : %s, port : %d", 
        _sockID, GetName(), host.c_str(), port);

    return true;
}

bool SocketTCPConnector::PreConnect(std::size_t idx)
{
    if (INVALID_RESULT == connect(_sock, (const sockaddr*)_cCtx->_addrs[idx]._addr, _cCtx->_addrs[idx]._len))
    {
        int err = errno;
        if (EINPROGRESS != err)
        {
            ZS_LOG_ERROR(network, "connect failed, sock id : %llu, socket name : %s, err : %d",
                _sockID, GetName(), err);
            return false;
        }
    }

    ZS_LOG_INFO(network, "pre connecting succeeded, socket id : %llu, socket name : %s", 
        _sockID, GetName());

    return true;
}

bool SocketTCPConnector::postConnect()
{
    return true;
}

#endif // defined(__GNUC__) || defined(__clang__)
