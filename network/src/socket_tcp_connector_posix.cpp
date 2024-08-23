
#include    "socket_tcp_connector.h"

#include    "common/log.h"

#include    "helper.h"
#include    "manager.h"

using namespace zs::common;
using namespace zs::network;

#if defined(_POSIX_) 

bool SocketTCPConnector::initConnect(std::size_t idx)
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

    ZS_LOG_INFO(network, "init connecting succeeded, socket id : %llu, socket name : %s", 
        _sockID, GetName());

    return true;
}

bool SocketTCPConnector::postConnect(bool& retry)
{
    int err;
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
    if (NO_ERROR != err)
    {
        if (CONN_REFUSED == err || CONN_TIMEOUT == err || 
            CONN_HOSTUNREACH == err || CONN_NETUNREACH == err)
        {
            ++_cCtx->_idx;
            if (_cCtx->_addrs.size() <= _cCtx->_idx)
            {
                ZS_LOG_ERROR(network, "no triable ip in post connect, sock id : %llu, socket name : %s", 
                    _sockID, GetName());
                return false;
            }

            if (false == initConnect(_cCtx->_idx))
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
        return false;
    }

    // if trying to connect never happens
    int32_t remotePort = 0;
    std::string remoteHost;
    Helper::GetSockRemoteAddr(_sock, _ipVer, remoteHost, remotePort);
    if (true == remoteHost.empty())
    {
        int err = errno;
        if (err == CONN_NOTCONN)
        {
            // ZS_LOG_WARN(network, "connection is not established yet in internal post connect, sock id : %llu, socket name : %s",
            //     _sockID, GetName());
            
            retry = true;
            return true;
        }

        ZS_LOG_ERROR(network, "GetSockRemoteAddr failed in internal post connect, sock id : %llu, socket name : %s, err : %d",
            _sockID, GetName(), err);
        return false;
    }

    // unbind the connected socket from dispatcher for connected event
    // bind the connected socket to dispatcher for data received event
    if (false == modifyBindingOnDispatcher(EventType::INBOUND))
    {
        ZS_LOG_ERROR(network, "binding connector socket on dispatcher failed, sock id : %llu, socket name : %s, peer : %s",
            _sockID, GetName(), GetPeer());
        return false;
    }

    return true;
}

#endif // defined(_POSIX_) 
